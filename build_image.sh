#!/bin/bash
set -e

MPY_VER=v1.13
ESP32_IDF_COMMIT=9e70825d1e1cbf7988cf36981774300066580ea7
ESP32_TOOLCHAIN="https://dl.espressif.com/dl/xtensa-esp32-elf-linux64-1.22.0-80-g6c4433a-5.2.0.tar.gz"

function msg() {
  echo -e "[\e[0;92m*\e[0m] $1"
}

if [ "$1" == "-h" ] || [ "$1" == "--help" ]; then
  echo "Usage: $0 [port] [board]"
  echo "Default: esp8266 GENERIC"
  exit
fi

# ESP8266 is the default for Wi-Se boards
port="esp8266"
board="GENERIC"
extra_make_args=()

if [ -n "$1" ]; then
  port="$1"
fi
if [ -n "$2" ]; then
  board="$2"
fi

if [ $port == "esp8266" ]; then
  msg "Using Docker"
  make=(docker run --rm -v "$PWD/micropython:$PWD/micropython" -u $UID -w "$PWD/micropython" larsks/esp-open-sdk make)

elif [ $port == "esp32" ]; then
  msg "Using downloaded ESP-IDF"

  make=(make)
  export ESPIDF="$PWD/esp-idf"

  if [ ! -d "$ESPIDF" ]; then
    msg "download ESP-IDF"
    git clone https://github.com/espressif/esp-idf.git "$ESPIDF"
    pushd "$ESPIDF"
    git checkout "$ESP32_IDF_COMMIT"
    git submodule update --init --recursive
    popd
  else
    msg "ESP-IDF found at $PWD/esp-idf"
  fi

  if [ ! -d build-esp32-venv ]; then
    msg "create ESP-IDF venv"

    python3 -m venv build-esp32-venv
    source build-esp32-venv/bin/activate
    pip install --upgrade pip wheel
    pip install -r "$ESPIDF/requirements.txt"
  else
    msg "ESP-IDF build venv found at $PWD/build-esp32-venv"
    source build-esp32-venv/bin/activate
  fi

  if [ ! -d xtensa-esp32-elf ]; then
    msg "download pre-built ESP32 toolchain"

    which wget 2>&1 && download=(wget -O -) || download=(curl)
    ${download[@]} "$ESP32_TOOLCHAIN" | tar -xz
  fi

  export PATH="$PWD/xtensa-esp32-elf/bin:$PATH"
  extra_make_args=(ESPIDF="$ESPIDF")
fi


cd "$(git rev-parse --show-toplevel)"

if [ ! -f "micropython/LICENSE" ]; then
  msg "clone micropython"
  git clone -b $MPY_VER --recursive https://github.com/micropython/micropython
else
  msg "micropython checkout"
  cd micropython
  git stash && git stash drop || true
  git checkout $MPY_VER
  cd ..
fi

cd micropython
msg "micropython submodules"
git submodule update --init --recursive

msg "mpy-cross"
"${make[@]}" -j$(nproc) -C mpy-cross

msg "wi_se module"
[ -d "ports/$port/modules/wi_se" ] && rm -Rf "ports/$port/modules/wi_se"
cp -a ../wi_se "ports/$port/modules/wi_se"

msg "mpy ports/$port submodules"
"${make[@]}" -j$(nproc) -C "ports/$port" submodules

msg "mpy ports/$port clean"
"${make[@]}" -j$(nproc) -C "ports/$port" submodules

msg "mpy ports/$port BOARD=$board ${extra_make_args[@]}"
"${make[@]}" -j$(nproc) -C "ports/$port" BOARD="$board" "${extra_make_args[@]}"

firmware_path="ports/$port/build-$board"

if [ -f "$firmware_path/firmware-combined.bin" ]; then
  msg "Success! Output firmware: micropython/$firmware_path/firmware-combined.bin"
elif [ -f "$firmware_path/firmware.bin" ]; then
  msg "Success! Output firmware: micropython/$firmware_path/firmware.bin"
else
  msg "Build failed."
fi
