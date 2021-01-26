# Wi-Se firmware - MicroPython version

Transparent bridge from ESP8266/ESP32 UART to web browser, over websockets.

It implements [ttyd](https://github.com/tsl0922/ttyd/)'s protcol and it uses
its web page.

While this does work, it suffers from heavy data losses at higher baud rates
due MicroPython being quite slow and due to the small hardware buffers (and
no software UART buffering done by MicroPython).

At 115200 bps there is minimal data loss on the ESP32, but the ESP8266 will
lose a lot of data even at 9600.

A rewrite in C with software buffering is being performed in the `c-lang`
branch to address this issue.


## Usage

You need to build MicroPython from source in order to freeze the bytecode into
it and run at an acceptable performance.

```bash
./build_image.sh esp8266    # requires docker!
./build_image.sh esp32      # downloads dependencies automatically
```

You can then flash the image with `esptool`, modify `wi_se_conf.template.py`
and upload it to `/wi_se_conf.py` using `ampy` or `rshell`.

Run the firmware with `import wi_se; wi_se.main()`, add it to `/boot.py` to
run automatically on boot.

The firmware disables the REPL console on start. To prevent it, press the
FLASH/BOOT0/IO0 button or short GPIO0 to ground a fraction of a second after
resetting the board.

## License

This software is licensed under the GNU General Public License v3.0.

The code under `/html` as well as `/wi_se/html.py` comes from `ttyd`, which is
licensed under the MIT license.

The `/wi_se/uwebsocket.py` file comes from [uwebsockets](https://github.com/danni/uwebsockets)
(with modification to work with asyncio), which is also licensed under the MIT license.

