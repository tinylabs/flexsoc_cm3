# Pin placement
set_property -dict { PACKAGE_PIN L17   IOSTANDARD LVCMOS33 } [get_ports { CLK_12M }];
set_property -dict { PACKAGE_PIN B18   IOSTANDARD LVCMOS33 } [get_ports { RESETn }];   # CK_RST

# UART
set_property -dict { PACKAGE_PIN J18   IOSTANDARD LVCMOS33 } [get_ports { UART_TX }]; # FPGA->HOST
set_property -dict { PACKAGE_PIN J17   IOSTANDARD LVCMOS33 } [get_ports { UART_RX }]; # HOST->FPGA

# Local JTAG/SWD
#set_property -dict { PACKAGE_PIN D15   IOSTANDARD LVCMOS33 } [get_ports { TCK_SWDCLK }]; # Pmod JB[3]
#set_property -dict { PACKAGE_PIN C15   IOSTANDARD LVCMOS33 } [get_ports { TMS_SWDIO }];  # Pmod JB[4]

# Target bridge connection
set_property -dict { PACKAGE_PIN G17   IOSTANDARD LVCMOS33 } [get_ports { BRG_SWDCLK }]; # Pmod JB[2]
set_property -dict { PACKAGE_PIN L18   IOSTANDARD LVCMOS33 } [get_ports { BRG_SWDIO }];  # Pmod JB[7]
# Set pullup to detect when disconnected
set_property pullup "TRUE" [get_ports { BRG_SWDIO }];

# Ignore timing on async reset
set_false_path -from [get_ports { RESETn }]

# Not sure how to calculate these, set dummy IO delays
set_input_delay -clock [get_clocks hclk] -add_delay 0.5 [get_ports RESETn]
#set_input_delay -clock [get_clocks jtag_clk] -add_delay 0.5 [get_ports TMS_SWDIO]
#set_output_delay -clock [get_clocks jtag_clk] -add_delay 0.5 [get_ports TMS_SWDIO]
set_input_delay -clock [get_clocks transport_clk] -add_delay 0.5 [get_ports UART_RX]
set_output_delay -clock [get_clocks transport_clk] -add_delay 0.5 [get_ports UART_TX]

# Increase JTAG speed
set_property BITSTREAM.GENERAL.COMPRESS TRUE [current_design]
set_property BITSTREAM.CONFIG.CONFIGRATE 33 [current_design]
