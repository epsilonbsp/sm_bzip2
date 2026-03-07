#!/usr/bin/env bash

clear

SDK=css

if [ "$1" = "install" ]; then
    mkdir -p build
    pushd build

    if [ ! -d sdk_root ]; then
        mkdir sdk_root
        pushd sdk_root

        git clone https://github.com/alliedmodders/sourcemod --recursive
        git clone https://github.com/alliedmodders/metamod-source
        git clone https://github.com/alliedmodders/ambuild

        python -m venv venv
        source venv/bin/activate
        python -m pip install ./ambuild
        deactivate

        popd
    fi

    popd
elif [ "$1" = "install_vendor" ]; then
    if [ ! -d build/vendor ]; then
        mkdir build/vendor
        pushd build/vendor

        git clone git://sourceware.org/git/bzip2.git

        popd
    fi
elif [ "$1" = "build" ]; then
    ROOT_PATH=$(pwd)
    mkdir -p build/output

    source build/sdk_root/venv/bin/activate

    pushd build/output

    python ../../configure.py --enable-optimize \
        --sm-path=$ROOT_PATH/build/sdk_root/sourcemod \
        --mms-path=$ROOT_PATH/build/sdk_root/metamod-source

    ambuild

    deactivate

    popd
else
    echo "No valid command was specified"
fi
