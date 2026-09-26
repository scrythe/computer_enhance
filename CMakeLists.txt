cmake_minimum_required(VERSION 4.3)
project(computer_enhance)

set(CMAKE_CXX_STANDARD 14)

include_directories(computer_enhance/perfaware/sim86)
include_directories(computer_enhance/perfaware/sim86/shared)
include_directories(computer_enhance/perfaware/sim86/shared/contrib_webassembly)
include_directories(src)

target_compile_options(my_target PRIVATE
        -fsanitize=address
        -fno-omit-frame-pointer
)

target_link_options(my_target PRIVATE
        -fsanitize=address
)

add_executable(computer_enhance
        computer_enhance/perfaware/sim86/shared/shared_library_test.cpp
        computer_enhance/perfaware/sim86/shared/sim86_shared.h
        computer_enhance/perfaware/sim86/shared/sim86_shared_debug.dll
        computer_enhance/perfaware/sim86/shared/sim86_shared_debug.lib
        computer_enhance/perfaware/sim86/shared/sim86_shared_debug.pdb
        computer_enhance/perfaware/sim86/shared/sim86_shared_release.dll
        computer_enhance/perfaware/sim86/shared/sim86_shared_release.lib
        computer_enhance/perfaware/sim86/shared/sim86_shared_release.pdb
        computer_enhance/perfaware/sim86/build.bat
        computer_enhance/perfaware/sim86/README.md
        computer_enhance/perfaware/sim86/sim86.cpp
        computer_enhance/perfaware/sim86/sim86.h
        computer_enhance/perfaware/sim86/sim86.rdbg
        computer_enhance/perfaware/sim86/sim86_cycles.cpp
        computer_enhance/perfaware/sim86/sim86_cycles.h
        computer_enhance/perfaware/sim86/sim86_decode.cpp
        computer_enhance/perfaware/sim86/sim86_decode.h
        computer_enhance/perfaware/sim86/sim86_execute.cpp
        computer_enhance/perfaware/sim86/sim86_execute.h
        computer_enhance/perfaware/sim86/sim86_instruction.cpp
        computer_enhance/perfaware/sim86/sim86_instruction.h
        computer_enhance/perfaware/sim86/sim86_instruction_table.cpp
        computer_enhance/perfaware/sim86/sim86_instruction_table.h
        computer_enhance/perfaware/sim86/sim86_instruction_table.inl
        computer_enhance/perfaware/sim86/sim86_instruction_table_standalone.h
        computer_enhance/perfaware/sim86/sim86_lib.cpp
        computer_enhance/perfaware/sim86/sim86_lib.h
        computer_enhance/perfaware/sim86/sim86_memory.cpp
        computer_enhance/perfaware/sim86/sim86_memory.h
        computer_enhance/perfaware/sim86/sim86_text.cpp
        computer_enhance/perfaware/sim86/sim86_text.h
        computer_enhance/perfaware/sim86/sim86_text_table.cpp
        src/sim86.cpp
        src/sim86.h)
