# Pin placement
set_property -dict { PACKAGE_PIN L17   IOSTANDARD LVCMOS33   HD.LOC_FIXED 1 } [get_ports { CLK_12M }];
set_property -dict { PACKAGE_PIN B18   IOSTANDARD LVCMOS33   HD.LOC_FIXED 1 } [get_ports { RESETn }];   # CK_RST

# UART
set_property -dict { PACKAGE_PIN J18   IOSTANDARD LVCMOS33   HD.LOC_FIXED 1 } [get_ports { UART_TX }]; # FPGA->HOST
set_property -dict { PACKAGE_PIN J17   IOSTANDARD LVCMOS33   HD.LOC_FIXED 1 } [get_ports { UART_RX }]; # HOST->FPGA

# Local JTAG/SWD
set_property -dict { PACKAGE_PIN M18   IOSTANDARD LVCMOS33 } [get_ports { TCK_SWDCLK }];
set_property -dict { PACKAGE_PIN U14   IOSTANDARD LVCMOS33 } [get_ports { TMS_SWDIO }];

# Target bridge connection
set_property -dict { PACKAGE_PIN H17   IOSTANDARD LVCMOS33 } [get_ports { BRG_SWDCLK }];
set_property -dict { PACKAGE_PIN H19   IOSTANDARD LVCMOS33 } [get_ports { BRG_SWDIO }];

# Set pullup to detect when disconnected
set_property pullup "TRUE" [get_ports { BRG_SWDIO }];

# Set IOSTANDARD for all ports
set_property IOSTANDARD LVCMOS33 [get_ports *]

# Autoplace below pins onto CMOD connector
#set_property prohibit 1 [get_bels *]
#set allowed_package_pins [list G17 G19 N18 L18 H17 H19 J19 K18]
#set allowed_bels [get_bels -of [get_sites [get_package_pins $allowed_package_pins]]]
#set_property prohibit 0 [get_bels $allowed_bels]

# Auto-place ports
# place_ports - can't run this in xdc

# Ignore timing on async reset
set_false_path -from [get_ports { RESETn }]

# Not sure how to calculate these, set dummy IO delays
set_input_delay -clock [get_clocks hclk] -add_delay 0.5 [get_ports RESETn]
set_input_delay -clock [get_clocks jtag_clk] -add_delay 0.5 [get_ports TMS_SWDIO]
set_output_delay -clock [get_clocks jtag_clk] -add_delay 0.5 [get_ports TMS_SWDIO]
set_input_delay -clock [get_clocks transport_clk] -add_delay 0.5 [get_ports UART_RX]
set_output_delay -clock [get_clocks transport_clk] -add_delay 0.5 [get_ports UART_TX]

# Increase JTAG speed
set_property BITSTREAM.GENERAL.COMPRESS TRUE [current_design]
set_property BITSTREAM.CONFIG.CONFIGRATE 33 [current_design]
