# Pin placement
set_property -dict { PACKAGE_PIN E3    IOSTANDARD LVCMOS33 } [get_ports { CLK_100M }];
set_property -dict { PACKAGE_PIN D9    IOSTANDARD LVCMOS33 } [get_ports { RESET }];   # BTN[0]

# JTAG/SWD
set_property -dict { PACKAGE_PIN T14   IOSTANDARD LVCMOS33 } [get_ports { TCK_SWDCLK }];  # IO[5]
set_property -dict { PACKAGE_PIN T15   IOSTANDARD LVCMOS33 } [get_ports { TMS_SWDIO }];   # IO[6]

# UART
set_property -dict { PACKAGE_PIN D10   IOSTANDARD LVCMOS33 } [get_ports { UART_TX }]; # FPGA->HOST
set_property -dict { PACKAGE_PIN A9    IOSTANDARD LVCMOS33 } [get_ports { UART_RX }]; # HOST->FPGA

# Ignore timing on async reset
set_false_path -from [get_ports { RESET }]

# Not sure how to calculate these, set dummy IO delays
set_input_delay -clock [get_clocks hclk] -add_delay 0.5 [get_ports RESET]
set_input_delay -clock [get_clocks jtag_clk] -add_delay 0.5 [get_ports TMS_SWDIO]
set_output_delay -clock [get_clocks jtag_clk] -add_delay 0.5 [get_ports TMS_SWDIO]
set_input_delay -clock [get_clocks transport_clk] -add_delay 0.5 [get_ports UART_RX]
set_output_delay -clock [get_clocks transport_clk] -add_delay 0.5 [get_ports UART_TX]

# Increase JTAG speed
set_property BITSTREAM.GENERAL.COMPRESS TRUE [current_design]
set_property BITSTREAM.CONFIG.CONFIGRATE 33 [current_design]
