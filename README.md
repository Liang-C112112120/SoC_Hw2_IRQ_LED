# SoC_Hw2_IRQ_LED
透過中斷控制LED  

# 一、查詢Zynq-7020 SoC技術手冊 ug585-Zynq-7000-TRM：  
1. IRQ numbers (ID)：
<img width="685" height="720" alt="IRQ numbers (ID)_1" src="https://github.com/user-attachments/assets/b057aabd-dcc7-4d47-ac3e-348b739172c6" />
<img width="690" height="328" alt="IRQ numbers (ID)_2" src="https://github.com/user-attachments/assets/c249fe45-d0eb-404d-9ff4-6a5403694848" />
  
2. Pending Register：  
<img width="667" height="684" alt="Pending Register" src="https://github.com/user-attachments/assets/58589798-5d5b-46ce-a2f3-09d8f17d62e2" />
<img width="643" height="788" alt="Pending Register (Address)" src="https://github.com/user-attachments/assets/d3ad84e0-ba43-4ddd-8700-b9ecc05b9ee7" />
  
3. Mask Register：
<img width="663" height="683" alt="Mask Register" src="https://github.com/user-attachments/assets/e21d18b3-93e1-4870-93f6-0e6e5b201168" />
<img width="631" height="790" alt="Mask Register (Address)" src="https://github.com/user-attachments/assets/a246d8d4-8a9b-4454-ab8a-7d0ff8096789" />
  
# 二、上板實作：  
輸入：五顆按鈕（BTNU、BTNC、BTND、BTNL、BTNR），對應不同的中斷輸入源與LED花樣、內建的AXI TIMER，總共六個中斷來源。  
輸出：八顆LED（LED0 ~ LED7）。  
中斷來源與LED花樣對應：BTNU二進位計數、BTNC跑馬燈左移、BTND跑馬燈右移、BTNL全部閃爍、BTNR跑馬燈來回掃描。  
  
按鈕與AXI TIMER都接到concat送進PS，Interrupt的Port ID由61依序分到66。  
在xparameter.h可以看到Vivado自動分好的Port ID：  
/* Definitions for Fabric interrupts connected to ps7_scugic_0 */  
#define XPAR_FABRIC_AXI_GPIO_1_IP2INTC_IRPT_INTR 61U //BTNU  
#define XPAR_FABRIC_AXI_TIMER_0_INTERRUPT_INTR 62U //AXI TIMER  
#define XPAR_FABRIC_AXI_GPIO_2_IP2INTC_IRPT_INTR 63U //BTNC  
#define XPAR_FABRIC_AXI_GPIO_4_IP2INTC_IRPT_INTR 64U //BTND  
#define XPAR_FABRIC_AXI_GPIO_3_IP2INTC_IRPT_INTR 65U //BTNL  
#define XPAR_FABRIC_AXI_GPIO_5_IP2INTC_IRPT_INTR 66U //BTNR  

1. 原版  
Demo影片：https://drive.google.com/file/d/1dGWucAJCdH49i7RmlI988M86nHPoXuw_/view?usp=sharing  
Block Design Diagram：  
<img width="1177" height="907" alt="Block Design Diagram" src="https://github.com/user-attachments/assets/4430a25e-677f-4fe3-8b3c-4528acc1ffbe" />
  
2. 增加按鈕防彈跳的RTL IP Block (button_debouncer)  
Block Design Diagram：  
<img width="1170" height="941" alt="Block Design Diagram 按鈕防彈跳" src="https://github.com/user-attachments/assets/912984cf-3c0f-458a-b1c8-c0e98708dd37" />
  
防彈跳原理：  
使用兩級D型正反器將非同步輸入的按鈕訊號做同步化，接著透過for迴圈對五個按鈕通道分別進行防彈跳處理。  
當輸入訊號與目前穩定輸出不同時，計數器開始累計；若訊號持續維持不同電位達到設定時間，則更新穩定輸出。若訊號恢復與穩定輸出相同，則計數器歸零，以此濾除按鈕彈跳造成輸出的短暫變化。  
<img width="1677" height="166" alt="button_debouncer 模擬" src="https://github.com/user-attachments/assets/f1a8b2c7-9d3b-499d-aef1-980d0885d0e6" />
