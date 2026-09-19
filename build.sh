#!/bin/bash

# inspired by https://github.com/EpicGames/raddebugger/blob/master/build.sh

set -eu
cd "$(dirname "$0")"

# --- Unpack Arguments --------------------------------------------------------
for arg in "$@"; do declare $arg='1'; done

mkdir -p build
cd build

if [[ ! -f "sim86_shared_debug.a" ]]; then
  echo "building sim86_shared_debug.a"
  clang++ -g -c -o sim86_shared_debug.o ../computer_enhance/perfaware/sim86/sim86_lib.cpp
  ar rs sim86_shared_debug.a sim86_shared_debug.o
fi

echo "building sim86.cpp"
clang++ -o sim86 ../src/sim86.cpp sim86_shared_debug.a

if [[ "${run:-0}" == "1" ]]; then
  echo "running sim86"
  ./sim86
fi

cd ..

# testing, will remove after commit
# clang++ -fPIC -shared computer_enhance/perfaware/sim86/sim86_lib.cpp -o libsim86_shared_debug.so
#
# clang++ -g -c -o sim86_shared_debug.o computer_enhance/perfaware/sim86/sim86_lib.cpp
# ar rs libsim86_shared_debug.a sim86_shared_debug.o

# clang++ -c computer_enhance/perfaware/sim86/sim86_lib.cpp -I computer_enhance/perfaware/sim86/shared -o libsim86_shared_debug.so
# clang++ -L . -lsim86_shared_debug src/sim86_a.cpp -o sim86
# clang++ -o sim86 src/sim86_a.cpp libsim86_shared_debug.a

# clang++ -Wno-unused-variable -g -O0 -DBUILD_DEBUG=1 -I computer_enhance/perfaware/sim86/shared src/sim86.cpp -lsim86_shared_debug -o sim86
# clang++ -Wno-unused-variable -g -O0 -DBUILD_DEBUG=1 -I computer_enhance/perfaware/sim86/shared src/sim86.cpp computer_enhance/perfaware/sim86/sim86_lib.cpp -o sim86
