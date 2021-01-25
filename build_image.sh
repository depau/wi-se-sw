#!/bin/bash
set -e

MPY_VER=v1.13

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

#if [ $port == "esp8266" ] && [ -f /etc/profile.d/xtensa-lx106-elf-gcc.sh ]; then
#  echo "Using "
#  source /etc/profile.d/xtensa-lx106-elf-gcc.sh
#  make=(make)
#else
msg "Using Docker"
make=(docker run --rm -v "$PWD/micropython:$PWD/micropython" -u $UID -w "$PWD/micropython" larsks/esp-open-sdk make)
#fi

if [ -n "$1" ]; then
  port="$1"
fi
if [ -n "$2" ]; then
  board="$2"
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

msg "mpy ports/$port BOARD=$board"
"${make[@]}" -j$(nproc) -C "ports/$port"

firmware="ports/$port/build-$board/firmware-combined.bin"

[ -f "$firmware" ] &&
  msg "Success! Output firmware: micropython/$firmware" ||
  msg "Build failed."
