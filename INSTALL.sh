#!/bin/bash

origin_path=$(dirname "$0"); pwd
cd "$(dirname "$0")" || exit 1

# create build directory
if [ ! -d "build/" ]; then
    mkdir build
fi

# go to build
cd build || exit 1

# cmake
flagdebug='-DCMAKE_BUILD_TYPE=Release'
flagarm64crypto='-DARM_CRYPTO=off'
while [ -n "$1" ]
do
    if [ "$1" = "debug" ]; then
        flagdebug="-DCMAKE_BUILD_TYPE=Debug"
    fi
    if [ "$1" = "arm64crypto" ]; then
        flagarm64crypto="-DARM_CRYPTO=on"
    fi
    shift
done

if ! cmake .. $flagdebug $flagarm64crypto;
then
    echo "cmake failed"
    cd "$origin_path" || exit 1
fi 

# make & install
os=$(uname)
if [ "$os" == "Darwin" ]; then
    cores=$(sysctl -n hw.logicalcpu)
    if [ "${cores}" == "" ]; then
        cores=1
    fi
    echo "make install -j${cores}"

    if ! make install -j${cores};
    then
        echo "make failed on Mac"
        exit 1 
    fi
else
    cores=$(nproc --all)
    if [ "${cores}" == "" ]; then
        cores=1
    fi
    echo "make -j${cores}"

    if ! make -j${cores};
    then
        echo "make failed on Linux"
        exit 1
    fi 

    echo "sudo make install"
    sudo make install
fi

cd "$origin_path" || exit 1
