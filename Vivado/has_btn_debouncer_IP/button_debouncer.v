`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 2026/11/03 02:51:22
// Design Name: 
// Module Name: button_debouncer
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

module button_debouncer #(
    parameter DEBOUNCE_CYCLES = 2000000
)(
    input  wire       clk,
    input  wire       rst,
    input  wire [4:0] btn_raw,
    output reg  [4:0] btn_clean
);

    // Counter width
    localparam COUNTER_WIDTH = $clog2(DEBOUNCE_CYCLES + 1);

    // Two-stage synchronizer
    reg [4:0] btn_sync_1;
    reg [4:0] btn_sync_2;

    // Independent debounce counters
    reg [COUNTER_WIDTH-1:0] count [0:4];

    integer i;

    // ----------------------------------
    // 1. Two-stage Synchronizer
    // ----------------------------------
    always @(posedge clk or posedge rst) begin
        if (rst) begin
            btn_sync_1 <= 5'b00000;
            btn_sync_2 <= 5'b00000;
        end
        else begin
            btn_sync_1 <= btn_raw;
            btn_sync_2 <= btn_sync_1;
        end
    end

    // ----------------------------------
    // 2. Five independent Debounce channels
    // ----------------------------------
    genvar ch;
    generate
        for (ch = 0; ch < 5; ch = ch + 1) begin : DEBOUNCE_CHANNEL

            always @(posedge clk or posedge rst) begin
                if (rst) begin
                    count[ch] <= 0;
                    btn_clean[ch] <= 1'b0;
                end
                else begin

                    // Input matches current stable output
                    if (btn_sync_2[ch] == btn_clean[ch]) begin
                        count[ch] <= 0;
                    end

                    // Input differs from current stable output
                    else begin
                        if (count[ch] >= DEBOUNCE_CYCLES - 1) begin
                            btn_clean[ch] <= btn_sync_2[ch];
                            count[ch] <= 0;
                        end
                        else begin
                            count[ch] <= count[ch] + 1'b1;
                        end
                    end
                end
            end

        end
    endgenerate

endmodule
