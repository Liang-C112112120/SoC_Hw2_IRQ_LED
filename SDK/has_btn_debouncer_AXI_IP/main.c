/*
 * main_button_ip.c
 *
 * 按鈕改由自訂 AXI IP (button_debouncer_axi_0) 提供：
 *   硬體去彈跳 + 按下事件旗標 + 5 條獨立的 IRQ (concat In0[4:0] -> IRQ_F2P[4:0] -> 61~65)
 *
 * 暫存器 (基底位址來自 xparameters.h):
 *   0x00 CTRL     保留 (目前去彈跳時間在 Verilog 以參數固定)
 *   0x04 STATE    唯讀：去彈跳後的 5 顆按鈕目前狀態
 *   0x08 PENDING  讀：按下事件旗標；寫 1 清除對應位元
 *   0x0C IRQ_EN   讀寫：中斷遮罩 (1 = 允許該顆按鈕送出中斷)
 *
 * 設計原則：ISR 只讀 PENDING、清旗標、改變數；LED 輸出與 UART 輸出都在 main 迴圈。
 */

#include <stdint.h>
#include "platform.h"
#include "sleep.h"
#include "xil_io.h"
#include "xil_printf.h"
#include "xil_assert.h"
#include "xstatus.h"
#include "xparameters.h"
#include "xgpio.h"
#include "xscugic.h"
#include "xtmrctr.h"

/* ===== 設定區 ===== */
#define USE_TIMER       1
#define USE_BUTTONS     1

#define NUM_BTN         5
#define TICK_HZ         5U      /* 動畫節拍：每秒 5 次 (0.2 秒) */

/* 按鈕 IP 的暫存器 */
#define BTN_BASE        XPAR_BUTTON_DEBOUNCER_AXI_0_S00_AXI_BASEADDR
#define REG_CTRL        0x00
#define REG_STATE       0x04
#define REG_PENDING     0x08
#define REG_IRQ_EN      0x0C
#define BTN_MASK_ALL    0x1FU
#define BTN_IRQ_FIRST   61U

#define LED_ID          XPAR_AXI_GPIO_0_DEVICE_ID
#define GIC_ID          XPAR_SCUGIC_SINGLE_DEVICE_ID

#if USE_TIMER
#define TIMER_ID        XPAR_AXI_TIMER_0_DEVICE_ID
#define INTC_TIMER_ID   XPAR_FABRIC_AXI_TIMER_0_INTERRUPT_INTR
/* Timer 往上數，載入 (2^32 - 每個節拍的時脈數)，溢位時發出中斷 */
#define TIMER_RESET_VALUE  ((u32)(0U - (XPAR_AXI_TIMER_0_CLOCK_FREQ_HZ / TICK_HZ)))
#endif

/* 初始化失敗時：印出步驟名稱與狀態碼，然後停住 */
#define CHECK(name, expr)                                              \
    do {                                                               \
        int _st = (expr);                                              \
        if (_st != XST_SUCCESS) {                                      \
            xil_printf("FAIL: %s (st=%d)\r\n", name, _st);             \
            while (1) { }                                              \
        }                                                              \
    } while (0)

/* ===== 全域變數 ===== */
XGpio     LED;
XScuGic   INTCInst;
XTmrCtr   TimerInst;

volatile int mode     = 0;    /* 目前的 LED 花樣 (ISR 改、main 讀)  */
volatile int tick     = 0;    /* Timer 每次中斷加 1，當作動畫時間   */
volatile int Intr_CTN = 0;    /* 有效按鈕事件的累計次數             */
volatile int last_btn = -1;   /* 最近一次按下的是哪一顆             */

static void Assert_Callback(const char8 *File, s32 Line)
{
    xil_printf("ASSERT FAIL: %s line %d\r\n", File, (int)Line);
}

/* ===== 中斷處理函式 ===== */
#if USE_BUTTONS
/* 5 條中斷線共用這一個函式，CallbackRef 帶的是「第幾顆按鈕」 */
static void Btn_Handler(void *CallbackRef)
{
    int i = (int)(uintptr_t)CallbackRef;
    u32 pending = Xil_In32(BTN_BASE + REG_PENDING);

    if (pending & (1U << i)) {
        Xil_Out32(BTN_BASE + REG_PENDING, 1U << i);   /* 寫 1 清除這一顆的旗標 */
        mode     = i;                                 /* 第 i 顆 -> 花樣 i     */
        tick     = 0;
        last_btn = i;
        Intr_CTN++;
    }
}
#endif

#if USE_TIMER
static void Timer_Callback(void *CallBackRef, u8 TmrCtrNumber)
{
    (void)CallBackRef;
    (void)TmrCtrNumber;
    tick++;
}
#endif

/* ===== 初始化函式 (成功回傳 XST_SUCCESS) ===== */
static int Led_Setup(void)
{
    int st = XGpio_Initialize(&LED, LED_ID);
    if (st != XST_SUCCESS) return st;

    XGpio_SetDataDirection(&LED, 1, 0x00);   /* CH1 全部輸出 */
    XGpio_DiscreteWrite(&LED, 1, 0x00);
    return XST_SUCCESS;
}

#if USE_BUTTONS
/* 先關閉遮罩並清掉殘留旗標，等中斷系統都綁好之後再開啟 */
static void Btn_Setup(void)
{
    Xil_Out32(BTN_BASE + REG_IRQ_EN,  0U);
    Xil_Out32(BTN_BASE + REG_PENDING, BTN_MASK_ALL);
}
#endif

#if USE_TIMER || USE_BUTTONS
static int Intr_Setup(void)
{
    int st;

    XScuGic_Config *cfg = XScuGic_LookupConfig(GIC_ID);
    if (cfg == NULL) return XST_FAILURE;

    st = XScuGic_CfgInitialize(&INTCInst, cfg, cfg->CpuBaseAddress);
    if (st != XST_SUCCESS) return st;

    Xil_ExceptionInit();
    Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
            (Xil_ExceptionHandler)XScuGic_InterruptHandler, &INTCInst);

#if USE_BUTTONS
    for (int i = 0; i < NUM_BTN; i++) {
        u32 id = BTN_IRQ_FIRST + (u32)i;

        /* IP 送出的是「高準位持續到 PS 清除」，GIC 要設成高準位觸發 */
        XScuGic_SetPriorityTriggerType(&INTCInst, id, 0xA0, 0x1);

        st = XScuGic_Connect(&INTCInst, id,
                (Xil_InterruptHandler)Btn_Handler, (void *)(uintptr_t)i);
        if (st != XST_SUCCESS) return st;
        XScuGic_Enable(&INTCInst, id);
    }
#endif

#if USE_TIMER
    st = XScuGic_Connect(&INTCInst, INTC_TIMER_ID,
            (Xil_InterruptHandler)XTmrCtr_InterruptHandler, &TimerInst);
    if (st != XST_SUCCESS) return st;
    XScuGic_Enable(&INTCInst, INTC_TIMER_ID);
#endif

    Xil_ExceptionEnableMask(XIL_EXCEPTION_IRQ);   /* 全部綁好之後，才讓 CPU 接受 IRQ */
    return XST_SUCCESS;
}
#endif

#if USE_TIMER
static int Timer_Setup(void)
{
    int st = XTmrCtr_Initialize(&TimerInst, TIMER_ID);
    if (st != XST_SUCCESS) return st;

    XTmrCtr_SetHandler(&TimerInst, Timer_Callback, &TimerInst);
    XTmrCtr_SetOptions(&TimerInst, 0,
            XTC_INT_MODE_OPTION | XTC_AUTO_RELOAD_OPTION);   /* 0 = Timer 0 */
    XTmrCtr_SetResetValue(&TimerInst, 0, TIMER_RESET_VALUE);
    XTmrCtr_Start(&TimerInst, 0);
    return XST_SUCCESS;
}
#endif

/* ===== LED 花樣：依花樣編號 m 與時間 t，算出 8 顆 LED 的圖樣 ===== */
u8 render(int m, int t)
{
    switch (m) {
    case 0: return (u8)(t & 0xFF);                  /* 二進位計數 */
    case 1: return (u8)(1 << (t % 8));              /* 跑馬燈 左移 */
    case 2: return (u8)(0x80 >> (t % 8));           /* 跑馬燈 右移 */
    case 3: return (t & 1) ? 0xFF : 0x00;           /* 全部閃爍 */
    case 4: {                                       /* 來回掃描 */
        int p = t % 14;
        if (p > 7) p = 14 - p;
        return (u8)(1 << p);
    }
    default: return 0x00;
    }
}

/* ===== main ===== */
int main(void)
{
    init_platform();
    Xil_AssertSetCallback(Assert_Callback);

    xil_printf("\r\n=== multi-IRQ LED demo (button AXI IP) ===\r\n");

    CHECK("LED setup", Led_Setup());
    XGpio_DiscreteWrite(&LED, 1, 0xAA);              /* 開機自我測試圖樣 */
    xil_printf("[1] LED ok\r\n");

#if USE_BUTTONS
    Btn_Setup();
    xil_printf("[2] button IP @0x%08x, STATE=0x%02x\r\n",
               (unsigned)BTN_BASE, (unsigned)(Xil_In32(BTN_BASE + REG_STATE) & BTN_MASK_ALL));
#endif

#if USE_TIMER || USE_BUTTONS
    CHECK("Interrupt setup", Intr_Setup());
    xil_printf("[3] interrupts ok\r\n");
#endif

#if USE_BUTTONS
    xil_printf("BTN IRQ IDs: %d..%d\r\n", (int)BTN_IRQ_FIRST, (int)(BTN_IRQ_FIRST + NUM_BTN - 1));
    Xil_Out32(BTN_BASE + REG_IRQ_EN, BTN_MASK_ALL);  /* 最後才開啟 IP 的中斷遮罩 */
#endif

#if USE_TIMER
    CHECK("Timer setup", Timer_Setup());
    xil_printf("[4] timer started (IRQ %d, reload 0x%08x)\r\n",
               (int)INTC_TIMER_ID, (unsigned)TIMER_RESET_VALUE);
#endif

    xil_printf("Init successful\r\n");

#if USE_BUTTONS
    int last_cnt = 0;
#endif

    while (1) {
#if USE_TIMER
        XGpio_DiscreteWrite(&LED, 1, render(mode, tick));
#endif

#if USE_BUTTONS
        if (Intr_CTN != last_cnt) {
            last_cnt = Intr_CTN;
            xil_printf("BTN%d -> mode %d (count %d)\r\n", last_btn, mode, last_cnt);
        }
#endif

        usleep(20000);   /* 20 ms */
    }

    return 0;
}
