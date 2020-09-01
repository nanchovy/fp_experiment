#!/bin/bash

ARCH_INC=../arch_dep/include

rm -r build
mkdir build
cd build

# -DCMAKE_C_FLAGS_DEBUG="-g3 -gdwarf-2" -DCMAKE_CXX_FLAGS_DEBUG="-g3 -gdwarf-2" 
cmake .. -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Prod \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON  -DARCH_INC_DIR=$ARCH_INC\
    $1
make clean
make -j8
