# LED0~LED7
set_property -dict {PACKAGE_PIN T22 IOSTANDARD LVCMOS33} [get_ports {leds_8bits_tri_o[0]}]
set_property -dict {PACKAGE_PIN T21 IOSTANDARD LVCMOS33} [get_ports {leds_8bits_tri_o[1]}]
set_property -dict {PACKAGE_PIN U22 IOSTANDARD LVCMOS33} [get_ports {leds_8bits_tri_o[2]}]
set_property -dict {PACKAGE_PIN U21 IOSTANDARD LVCMOS33} [get_ports {leds_8bits_tri_o[3]}]
set_property -dict {PACKAGE_PIN V22 IOSTANDARD LVCMOS33} [get_ports {leds_8bits_tri_o[4]}]
set_property -dict {PACKAGE_PIN W22 IOSTANDARD LVCMOS33} [get_ports {leds_8bits_tri_o[5]}]
set_property -dict {PACKAGE_PIN U19 IOSTANDARD LVCMOS33} [get_ports {leds_8bits_tri_o[6]}]
set_property -dict {PACKAGE_PIN U14 IOSTANDARD LVCMOS33} [get_ports {leds_8bits_tri_o[7]}]

# BTNU, BTNC, BTND, BTNL, BTNR (btns_5bits[4] = BTNR ... btns_5bits[0] = BTNU)
set_property -dict {PACKAGE_PIN T18 IOSTANDARD LVCMOS33} [get_ports {btns_5bits[0]}]
set_property -dict {PACKAGE_PIN P16 IOSTANDARD LVCMOS33} [get_ports {btns_5bits[1]}]
set_property -dict {PACKAGE_PIN R16 IOSTANDARD LVCMOS33} [get_ports {btns_5bits[2]}]
set_property -dict {PACKAGE_PIN N15 IOSTANDARD LVCMOS33} [get_ports {btns_5bits[3]}]
set_property -dict {PACKAGE_PIN R18 IOSTANDARD LVCMOS33} [get_ports {btns_5bits[4]}]
