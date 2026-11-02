/*
 * 逐步除錯用的開關：
 *   USE_TIMER=0, USE_BUTTONS=0 : LED自我測試
 *   USE_TIMER=1, USE_BUTTONS=0 : LED以二進位計數
 *   USE_TIMER=1, USE_BUTTONS=1 : 完整功能
 */

#include "platform.h"
#include "sleep.h"
#include "xil_printf.h"
#include "xil_assert.h"
#include "xstatus.h"
#include "xparameters.h"
#include "xgpio.h"
#include "xscugic.h"
#include "xtmrctr.h"

#define USE_TIMER           1       //1: 啟用Timer中斷(動畫節拍)
#define USE_BUTTONS         1       //1: 啟用5顆按鈕中斷
#define BTN_PRESSED_LEVEL   1       //按鈕按下時讀到的電位

#define NUM_BTN             5
#define TIMER_RESET_VALUE   0xFF676980U //PL時脈50MHz時約0.2秒

#define LED_ID              XPAR_AXI_GPIO_0_DEVICE_ID
#define GIC_ID              XPAR_SCUGIC_SINGLE_DEVICE_ID
#define BTN_INT             XGPIO_IR_CH1_MASK

#if USE_TIMER
#define TIMER_ID            XPAR_AXI_TIMER_0_DEVICE_ID
#define INTC_TIMER_ID       XPAR_FABRIC_AXI_TIMER_0_INTERRUPT_INTR
#endif

#if USE_BUTTONS
//5顆按鈕各自的GPIO裝置編號(xparameters.h)
static const u16 BTN_DEV_ID[NUM_BTN] = {
    XPAR_AXI_GPIO_1_DEVICE_ID, XPAR_AXI_GPIO_2_DEVICE_ID,
    XPAR_AXI_GPIO_3_DEVICE_ID, XPAR_AXI_GPIO_4_DEVICE_ID,
    XPAR_AXI_GPIO_5_DEVICE_ID
};

//5顆按鈕各自的中斷編號(xparameters.h)
static const u32 BTN_INTR_ID[NUM_BTN] = {
    XPAR_FABRIC_AXI_GPIO_1_IP2INTC_IRPT_INTR, XPAR_FABRIC_AXI_GPIO_2_IP2INTC_IRPT_INTR,
    XPAR_FABRIC_AXI_GPIO_3_IP2INTC_IRPT_INTR, XPAR_FABRIC_AXI_GPIO_4_IP2INTC_IRPT_INTR,
    XPAR_FABRIC_AXI_GPIO_5_IP2INTC_IRPT_INTR
};
#endif

//初始化失敗時：印出步驟名稱與狀態碼，然後停住
#define CHECK(name, expr)                                              \
    do {                                                               \
        int _st = (expr);                                              \
        if (_st != XST_SUCCESS) {                                      \
            xil_printf("FAIL: %s (st=%d)\r\n", name, _st);             \
            while (1) { }                                              \
        }                                                              \
    } while (0)


typedef struct {
    XGpio gpio;       //這顆按鈕的GPIO實例
    int   pattern_id; //按下後要切換到的LED花樣編號
} BtnCtx;

BtnCtx    btn[NUM_BTN];
XGpio     LED;
XScuGic   INTCInst;
XTmrCtr   TimerInst;

volatile int mode     = 0;   //目前的LED花樣
volatile int tick     = 0;   //每次中斷加1當作動畫時間
volatile int Intr_CTN = 0;   //有效按鈕事件的累計次數
volatile int last_btn = -1;  //最近一次按下的是哪一顆

//斷言失敗時印出檔名與行號 (印完後 Xil_Assert 會停在無窮迴圈)
static void Assert_Callback(const char8 *File, s32 Line)
{
    xil_printf("ASSERT FAIL: %s line %d\r\n", File, (int)Line);
}

//中斷處理函式
#if USE_BUTTONS
//按鈕中斷：5條中斷線共用，用CallbackRef區分是哪一顆
static void Btn_Handler(void *CallbackRef)
{
    BtnCtx *b = (BtnCtx *)CallbackRef;

    XGpio_InterruptDisable(&b->gpio, BTN_INT);
    XGpio_InterruptClear(&b->gpio, BTN_INT);

    //按下與放開都會觸發中斷，只處理按下那一次
    if ((XGpio_DiscreteRead(&b->gpio, 1) & 0x1) == BTN_PRESSED_LEVEL) {
        mode     = b->pattern_id;
        tick     = 0;
        last_btn = b->pattern_id;
        Intr_CTN++;
    }

    XGpio_InterruptEnable(&b->gpio, BTN_INT);
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

//初始化函式 (成功回傳 XST_SUCCESS)
static int Led_Setup(void)
{
    int st = XGpio_Initialize(&LED, LED_ID);
    if (st != XST_SUCCESS) return st;

    XGpio_SetDataDirection(&LED, 1, 0x00); //CH1輸出
    XGpio_DiscreteWrite(&LED, 1, 0x00);
    return XST_SUCCESS;
}

#if USE_BUTTONS
static int Btn_Setup(void)
{
    for (int i = 0; i < NUM_BTN; i++) {
        int st = XGpio_Initialize(&btn[i].gpio, BTN_DEV_ID[i]);
        if (st != XST_SUCCESS) return st;

        XGpio_SetDataDirection(&btn[i].gpio, 1, 0x1); //CH1輸入
        btn[i].pattern_id = i;                        //第i顆對應花樣i
    }
    return XST_SUCCESS;
}
#endif

#if USE_TIMER || USE_BUTTONS
static int Intr_Setup(void)
{
    int st;

    //初始化GIC
    XScuGic_Config *cfg = XScuGic_LookupConfig(GIC_ID);
    if (cfg == NULL) return XST_FAILURE;

    st = XScuGic_CfgInitialize(&INTCInst, cfg, cfg->CpuBaseAddress);
    if (st != XST_SUCCESS) return st;

    //註冊CPU的IRQ入口
    Xil_ExceptionInit();
    Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
            (Xil_ExceptionHandler)XScuGic_InterruptHandler, &INTCInst);

#if USE_BUTTONS
    //5顆按鈕各自綁一條中斷
    for (int i = 0; i < NUM_BTN; i++) {
        st = XScuGic_Connect(&INTCInst, BTN_INTR_ID[i],
                (Xil_InterruptHandler)Btn_Handler, &btn[i]);
        if (st != XST_SUCCESS) return st;
        XScuGic_Enable(&INTCInst, BTN_INTR_ID[i]);

        XGpio_InterruptClear(&btn[i].gpio, BTN_INT);
        XGpio_InterruptEnable(&btn[i].gpio, BTN_INT);
        XGpio_InterruptGlobalEnable(&btn[i].gpio);
    }
#endif

#if USE_TIMER
    //Timer綁Xilinx的標準處理函式，它會再呼叫Timer_Callback
    st = XScuGic_Connect(&INTCInst, INTC_TIMER_ID,
            (Xil_InterruptHandler)XTmrCtr_InterruptHandler, &TimerInst);
    if (st != XST_SUCCESS) return st;
    XScuGic_Enable(&INTCInst, INTC_TIMER_ID);
#endif

    //全部綁好之後，才讓CPU接受IRQ
    Xil_ExceptionEnableMask(XIL_EXCEPTION_IRQ);
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

//LED 花樣：依花樣編號m與時間t來算出8顆LED的圖樣
u8 render(int m, int t)
{
    switch (m) {
    case 0: return (u8)(t & 0xFF);        //二進位計數
    case 1: return (u8)(1 << (t % 8));    //跑馬燈 左移
    case 2: return (u8)(0x80 >> (t % 8)); //跑馬燈 右移
    case 3: return (t & 1) ? 0xFF : 0x00; //全部閃爍
    case 4: {                             //來回掃描
        int p = t % 14;
        if (p > 7) p = 14 - p;
        return (u8)(1 << p);
    }
    default: return 0x00;
    }
}

//main
int main(void)
{
    init_platform();
    Xil_AssertSetCallback(Assert_Callback);

    xil_printf("\r\n=== multi-IRQ LED demo ===\r\n");

    CHECK("LED setup", Led_Setup());
    XGpio_DiscreteWrite(&LED, 1, 0xAA);              /* 開機自我測試圖樣 */
    xil_printf("[1] LED ok\r\n");

#if USE_BUTTONS
    CHECK("Button setup", Btn_Setup());
    xil_printf("[2] buttons ok\r\n");
#endif

#if USE_TIMER || USE_BUTTONS
    CHECK("Interrupt setup", Intr_Setup());
    xil_printf("[3] interrupts ok\r\n");
#endif

#if USE_BUTTONS
    xil_printf("BTN IRQ IDs: %d %d %d %d %d\r\n",
               (int)BTN_INTR_ID[0], (int)BTN_INTR_ID[1], (int)BTN_INTR_ID[2],
               (int)BTN_INTR_ID[3], (int)BTN_INTR_ID[4]);
#endif

#if USE_TIMER
    CHECK("Timer setup", Timer_Setup());
    xil_printf("[4] timer started (IRQ %d)\r\n", (int)INTC_TIMER_ID);
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
