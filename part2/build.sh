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
  elif [[ $1 == "-file="* ]]; then
    file="${1#-file=}"
    shift
  else
    declare $1='1'
    shift
  fi
done

if [[ "${clean:-0}" == 1 ]]; then
  echo "[clean build folder]"
  rm -rf build
fi

mkdir -p build

asan_flags=""
if [[ "${asan:-0}" == 1 ]]; then
  echo "[enable asan]"
  asan_flags="-fsanitize=address -fno-omit-frame-pointer"
fi

if [[ ! -f "haversine_formula.a" ]]; then
  echo "[building sim86_shared_debug.a]"
  clang -g $asan_flags -c -o build/haversine_formula.o src/haversine_formula.cpp
  llvm-ar rs build/haversine_formula.a build/haversine_formula.o
fi

if [[ ! -e "src/$file.c" ]]; then
  echo "file 'src/$file' does not exist"
  exit 1
fi

if [[ $file != *"_main" ]]; then
  echo "file 'src/$file' is not executable"
  exit 1
fi

common_build_flags="-g -std=c99 $asan_flags  -Wall -Werror -o build/$file src/$file.c build/haversine_formula.a -lm"
if [[ "${release:-0}" == 1 ]]; then
  echo "[release mode]"
  compile="clang -O2 $common_build_flags"
else
  echo "[debug mode]"
  compile="clang -O0 $common_build_flags -Wno-unused-variable -Wno-unused-but-set-variable -DBUILD_DEBUG=1"
fi

echo "[building $file]"
$compile

if [[ "${run:-0}" == "1" ]]; then
  echo "[running $file]"
  ./build/$file $command_args
fi
