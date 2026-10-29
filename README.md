# SoC_Hw2_IRQ_LED
透過中斷控制LED  

1. 查詢Zynq-7020 SoC技術手冊ug585-Zynq-7000-TRM (2014)：  
IRQ numbers (ID)：  
<img width="686" height="727" alt="IRQ numbers (ID)_1" src="https://github.com/user-attachments/assets/33e6de27-7fb2-4c66-a6cf-8143ca9dd7a4" />
<img width="681" height="310" alt="IRQ numbers (ID)_2" src="https://github.com/user-attachments/assets/c65a82e0-540e-46f2-be3a-9b522d712aa7" />

Interrupt Controller Register：  
<img width="666" height="686" alt="Interrupt Controller Register_Overview" src="https://github.com/user-attachments/assets/eaa00813-76a6-4ecf-bc08-35aff576cf84" />
<img width="644" height="873" alt="Interrupt Controller Register_Address_1" src="https://github.com/user-attachments/assets/9471fc0d-18cc-4ea0-9715-0b814208fc85" />
<img width="640" height="865" alt="Interrupt Controller Register_Address_2" src="https://github.com/user-attachments/assets/98536175-6563-4125-91b1-65c47ed2537e" />
<img width="643" height="874" alt="Interrupt Controller Register_Address_3" src="https://github.com/user-attachments/assets/351de809-2de6-4a17-b725-3494c1704ed8" />
<img width="635" height="334" alt="Interrupt Controller Register_Address_4" src="https://github.com/user-attachments/assets/a78393a5-9b29-4429-a8fa-3f39a9ee1a82" />

Pending Register： 
PPI(Private Peripheral Interrupts) and SPI(Shared Peripheral Interrupts) Status：  
<img width="670" height="690" alt="Pending Register (PPI   SPI)" src="https://github.com/user-attachments/assets/bd3713b1-6fde-4515-916e-5d59f4148ce9" />

Mask Register：
e.g. I2C Controller：
<img width="650" height="566" alt="Mask Register (e g  I2C Controller)" src="https://github.com/user-attachments/assets/106e30ea-31d1-426f-b62c-4f4b2b38976d" />

2. 上板實作：
