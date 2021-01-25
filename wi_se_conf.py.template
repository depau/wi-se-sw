## Board type
# Use custom for a generic board.
# Use the following if you have a Wi-Se board:
# - wi-se-rpi-v0.1
# - wi-se-opi4-v0.1
# - wi-se-rewirable-v0.1
board_type = "wi-se-rewirable-v0.1"

## LED configuration for custom boards
# LEDs are pre-configured on Wi-Se boards
# led_wifi = const(5)
# led_status = const(6)
# led_rx = const(7)
# led_tx = const(8)

# Uncomment and set to True if your LEDs are active-low
# led_wifi_invert = False
# led_status_invert = False
# led_rx_invert = False
# led_tx_invert = False

## Wi-Fi configuration

# Wi-Fi mode: sta (client) or ap (access-point)
wifi_mode = "ap"

# Device hostname
hostname = "wi-se"

# Wi-Fi name and password
wifi_ssid = "Wi-Se"
wifi_key = "veryl33t"

# Wi-Fi parameters for AP mode - ignored in sta mode
# Authentication mode
# - open (DON'T)
# - wep (DON'T)
# - wpa_psk (avoid)
# - wpa2_psk
# - wpa_wpa2_psk (best compromise, avoid if possible)
wifi_authmode = "wpa2_psk"

## Server configuration
http_listen = "0.0.0.0"
http_port = const(80)
http_basic_auth = "user:pass"  # None to disable

# WebSocket ping interval, default is 300s
ws_ping_interval = const(300)

## Web TTY configuration
# You can specify any option documented here: https://xtermjs.org/docs/api/terminal/interfaces/iterminaloptions/
ttyd_web_conf = {
    'disableLeaveAlert': True
}

## Firewall configuration
# Only allow connections from private IP addresses
private_nets_only = True

## UART configuration
# UART port to use - Use 0 on Wi-Se boards.
uart_id = const(0)

# UART default parameters
uart_baud = const(115200)
uart_bits = const(8)
uart_parity = None
uart_stop = const(1)

# Whether to unbind REPL. If you want to use the main UART for the server, it must be unbound.
# This must be true on Wi-Se boards
unbind_repl = True
