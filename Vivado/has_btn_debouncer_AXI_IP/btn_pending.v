`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 2026/11/05 14:51:09
// Design Name: 
// Module Name: btn_pending
// Project Name: 
// Target Devices: 
// Tool Versions: 
// Description: 
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////

// 按下事件旗標：偵測 btn_clean 上緣 -> 設定 pending；PS 寫 1 清除；irq = pending & irq_en
module btn_pending (
    input  wire       clk,
    input  wire       rst,          // 高態有效 (AXI 的 aresetn 要反相後接進來)
    input  wire [4:0] btn_clean,    // 來自 button_debouncer
    input  wire [4:0] irq_en,       // 中斷遮罩
    input  wire       clr_strobe,   // PS 寫 PENDING 暫存器時為 1 (一個週期)
    input  wire [4:0] clr_mask,     // 寫入資料；為 1 的位元會被清除
    output reg  [4:0] pending,
    output wire [4:0] irq
);
    reg  [4:0] btn_d;
    wire [4:0] press = btn_clean & ~btn_d;                 // 只在 0->1 的那一個週期為 1
    wire [4:0] clr   = clr_strobe ? clr_mask : 5'b00000;

    always @(posedge clk or posedge rst) begin
        if (rst) begin
            btn_d   <= 5'b00000;
            pending <= 5'b00000;
        end else begin
            btn_d   <= btn_clean;
            pending <= (pending & ~clr) | press;           // 同時發生時，set 優先於 clear
        end
    end

    assign irq = pending & irq_en;                         // 高準位持續到 PS 清除
endmodule
