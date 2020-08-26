# Pin placement
set_property -dict { PACKAGE_PIN E3    IOSTANDARD LVCMOS33 } [get_ports { CLK_100M }];
set_property -dict { PACKAGE_PIN D9    IOSTANDARD LVCMOS33 } [get_ports { RESET }];   # BTN[0]

# JTAG
set_property -dict { PACKAGE_PIN P17   IOSTANDARD LVCMOS33 } [get_ports { TCK_SWDCLK }];  # IO13
set_property -dict { PACKAGE_PIN R17   IOSTANDARD LVCMOS33 } [get_ports { TDI }];         # IO12
set_property -dict { PACKAGE_PIN U18   IOSTANDARD LVCMOS33 } [get_ports { TDO }];         # IO11
set_property -dict { PACKAGE_PIN V17   IOSTANDARD LVCMOS33 } [get_ports { TMS_SWDIO }];   # IO10

# UART
set_property -dict { PACKAGE_PIN D10   IOSTANDARD LVCMOS33 } [get_ports { UART_TX }]; # FPGA->HOST
set_property -dict { PACKAGE_PIN A9    IOSTANDARD LVCMOS33 } [get_ports { UART_RX }]; # HOST->FPGA

# Ignore timing on async reset
set_false_path -from [get_ports { RESET }]

# Untimed ports
set untimed_od 0.5
set untimed_id 0.5
set_input_delay  -clock [get_clocks hclk] -add_delay $untimed_id [get_ports RESET]

# Ignore RX path, we oversample x4 and constrain max baud
set_false_path -from [get_ports UART_RX]
set_output_delay -clock [get_clocks transport_clk] -add_delay $untimed_id [get_ports UART_TX]
#set_input_delay -clock [get_clocks transport_clk] -add_delay $untimed_id [get_ports UART_RX]
