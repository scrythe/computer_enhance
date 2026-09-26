#!/bin/bash

# inspired by https://github.com/EpicGames/raddebugger/blob/master/build.sh

set -eu
cd "$(dirname "$0")"
command_args=""

# --- Unpack Arguments --------------------------------------------------------
while [[ $# -gt 0 ]]; do
  if [[ "$1" == "--" ]]; then
    shift
    command_args=$@
    break
  else
    declare $1='1'
    shift
  fi
done

if [[ "${clean_build:-0}" == 1 ]]; then
  echo "[clean build folder]"
  rm -rf build
fi

mkdir -p build

asan_flags=""
if [[ "${asan:-0}" == 1 ]]; then
  echo "[enable asan]"
  asan_flags="-fsanitize=address -fno-omit-frame-pointer"
fi

common_build_flags="-g $asan_flags -Wall -Werror -o build/sim86 src/sim86.cpp build/sim86_shared_debug.a"
if [[ "${release:-0}" == 1 ]]; then
  echo "[release mode]"
  compile="clang -O2 $common_build_flags"
else
  echo "[debug mode]"
  compile="clang -O0 $common_build_flags -Wno-unused-variable -Wno-unused-but-set-variable -DBUILD_DEBUG=1"
fi

if [[ ! -f "sim86_shared_debug.a" ]]; then
  echo "[building sim86_shared_debug.a]"
  clang++ -g $asan_flags -c -o build/sim86_shared_debug.o computer_enhance/perfaware/sim86/sim86_lib.cpp
  llvm-ar rs build/sim86_shared_debug.a build/sim86_shared_debug.o
fi

echo "[building sim86]"
$compile

if [[ "${run:-0}" == "1" ]]; then
  echo "[running sim86]"
  ./build/sim86 $command_args
fi
