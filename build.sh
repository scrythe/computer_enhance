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
  rm -rf build
fi

mkdir -p build
cd build

if [[ "${release:-0}" == 1 ]]; then
  echo "[release mode]"
  compile="clang -g -O2                                                    -o sim86 ../src/sim86.cpp sim86_shared_debug.a"
else
  echo "[debug mode]"
  compile="clang -g -O0 -DBUILD_DEBUG=1 -Wall -Werror -Wno-unused-variable -o sim86 ../src/sim86.cpp sim86_shared_debug.a"
fi

if [[ ! -f "sim86_shared_debug.a" ]]; then
  echo "[building sim86_shared_debug.a]"
  clang -g -c -o sim86_shared_debug.o ../computer_enhance/perfaware/sim86/sim86_lib.cpp
  llvm-ar rs sim86_shared_debug.a sim86_shared_debug.o
fi

echo "[building sim86]"
$compile

if [[ "${run:-0}" == "1" ]]; then
  echo "[running sim86]"
  ./sim86 $command_args
fi

cd ..
