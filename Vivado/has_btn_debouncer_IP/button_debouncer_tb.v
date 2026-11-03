`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 2026/11/03 02:57:08
// Design Name: 
// Module Name: button_debouncer_tb
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

`timescale 1ns / 1ps

module button_debouncer_tb;

    reg clk;
    reg rst;
    reg [4:0] btn_raw;
    wire [4:0] btn_clean;

    // Use 5 cycles for simulation
    button_debouncer #(
        .DEBOUNCE_CYCLES(5)
    ) uut (
        .clk(clk),
        .rst(rst),
        .btn_raw(btn_raw),
        .btn_clean(btn_clean)
    );

    // 100 MHz clock (10 ns period)
    initial begin
        clk = 0;
        forever #5 clk = ~clk;
    end

    // Test procedure
    initial begin

        $display("--------------------------------");
        $display(" Button Debounce Test Start");
        $display("--------------------------------");

        rst = 1;
        btn_raw = 5'b00000;

        repeat (3) @(posedge clk);
        @(negedge clk);
        rst = 0;

        // --------------------------------
        // Test 1: Short HIGH bounce
        // --------------------------------
        $display("Test 1: Short HIGH bounce");

        btn_raw[0] = 1'b1;
        repeat (1) @(posedge clk);
        @(negedge clk);
        btn_raw[0] = 1'b0;

        repeat (5) @(posedge clk);
        #1;

        if (btn_clean[0] == 1'b0)
            $display("PASS: Short HIGH ignored");
        else
            $display("FAIL: Short HIGH detected");

        // --------------------------------
        // Test 2: Stable HIGH
        // --------------------------------
        $display("Test 2: Stable HIGH");

        @(negedge clk);
        btn_raw[0] = 1'b1;

        repeat (10) @(posedge clk);
        #1;

        if (btn_clean[0] == 1'b1)
            $display("PASS: Stable HIGH detected");
        else
            $display("FAIL: Stable HIGH not detected");

        // --------------------------------
        // Test 3: Short LOW bounce
        // --------------------------------
        $display("Test 3: Short LOW bounce");

        @(negedge clk);
        btn_raw[0] = 1'b0;

        repeat (1) @(posedge clk);
        @(negedge clk);
        btn_raw[0] = 1'b1;

        repeat (5) @(posedge clk);
        #1;

        if (btn_clean[0] == 1'b1)
            $display("PASS: Short LOW ignored");
        else
            $display("FAIL: Short LOW detected");

        // --------------------------------
        // Test 4: Stable LOW
        // --------------------------------
        $display("Test 4: Stable LOW");

        @(negedge clk);
        btn_raw[0] = 1'b0;

        repeat (10) @(posedge clk);
        #1;

        if (btn_clean[0] == 1'b0)
            $display("PASS: Stable LOW detected");
        else
            $display("FAIL: Stable LOW not detected");

        // --------------------------------
        // Test 5: Independent channels
        // --------------------------------
        $display("Test 5: Independent channels");

        @(negedge clk);
        btn_raw[1] = 1'b1;

        repeat (10) @(posedge clk);
        #1;

        if (btn_clean[1] == 1'b1 &&
            btn_clean[0] == 1'b0)
            $display("PASS: Channels operate independently");
        else
            $display("FAIL: Channel independence error");

        $display("--------------------------------");
        $display(" Button Debounce Test Finished");
        $display("--------------------------------");

        $finish;
    end

endmodule
