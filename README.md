# Wi-Se Remote UART Terminal - C++ implementation

> *Wireless Serial*

![Demo](https://i.postimg.cc/PqRRcwX1/ezgif-com-optimize-1.gif)

This software allows for using an ESP8266 board as a remote UART terminal.
It is very fast, reaching (with [caveats](#caveats)) up to 1500000bps rates.

It is intended as a firmware for the [Wi-Se](https://github.com/Depau/wi-se-hw/)
boards (which you can order and build at your favorite PCB manufacturer), though
it will work just fine with a normal ESP8266 breakout board.

It should work with ESP32 as well but it may need some tweaks. It hasn't been
tested.

Communication occurs over WebSockets: it is compatible with
[ttyd](https://github.com/tsl0922/ttyd/). In fact, the web UI is *exactly* the
same. CLI clients compatible with ttyd should also work with Wi-Se.


## Features

- Web-based terminal based on Xterm.js
- Very low latency (~10ms on average, depending on your Wi-Fi)
- Relatively high baud rates are supported (up to 150000bps, with 
  [caveats](#caveats))
- Zmodem support
- Native *nix client that spawns a pseudo-terminal device (to be written)


## Usage

The project uses ESP8266 Arduino core, but it is intended to be built with
[PlatformIO](https://docs.platformio.org/en/latest/index.html).

You can find instructions on how to install it on their website.
You can also find instructions on how to build and run the code with their
VSCode-based IDE on their website.

Though if you don't want to install the entire crap, you can just install
PlatformIO Core (command line), then build and upload the project with:

```bash
pio run --target upload
```

But before doing that, make sure you specify your device's serial port in
`platformio.ini`. Look for `upload_port`.

Also make sure you tweak the configuration, as described in the next section.


## Configuration

Configuration is located in `include/config.h`. You have to modify it before
flashing.

Inside you will find options such as Wi-Fi client/softAP mode, network name and
password, as well as the UART parameters.

Debug output is redirected by default to UART1. You can also set both UARTs to
the same port and it will print both WebSocket data and debug output to that
port. Only enable it for debugging, since it will send loads of crap to the
connected terminal.


## Changing UART parameters at runtime

It will eventually be implemented in the web page. For now, you can change the
baud rate and the other parameters by sending an HTTP request:

```bash
curl -X POST IP_ADDRESS/stty -H 'Content-Type: application/json' \
-d '{"baudrate":1500000,"bits":8,"parity":null,"stop":1}'

# To view:
curl IP_ADDRESS/stty
```

Bits can be `5`, `6`, `7` or `8`, and it must not be `8` if parity is not none.

Parity can be `null` (none), `0` (even), `1` (odd).

Stop can be `0`, `1`, `15`, `2`.

Defaults (`8`, `null`, `1`) will work for most setups.

You also may provide only the parameters you want to change, for example
`{"baudrate": 115200}`.


## Caveats

ESP8266 has incredible capabilities, but fast Wi-Fi isn't one of them.

While it can and it will reach up to 1500000 bps, it will only do so if:

- You DO NOT use the UART to USB adapter built into most devkits (but rather
  a better external adapter such as the FTDI FT2232H)
	- The built-in adapter can't go faster than ~500000bps
- You DO NOT send a constant stream of data at high speeds

Sending a continuous stream of data (for example running Zmodem, `nyancat`,
similar terminal-based animations or in general any application that sends more
than approx. two screens of data at a time without pausing) will result in
truncation.

The default buffering settings have been tuned to be able to run "normal" 
terminal software, such as vim, htop, nmtui, more/less, cdialog-based apps, etc.

Zmodem will almost certainly crash and lock up your terminal at high rates.


## License

This project is licensed under the GNU General Public License v3.0.

All content under `/html` was originally written for
[ttyd](https://github.com/tsl0922/ttyd/), and it has been slightly modified.
[ttyd](https://github.com/tsl0922/ttyd/) is licensed under MIT license.

See the git commit history for the `/html` directory for original authors
credits.





