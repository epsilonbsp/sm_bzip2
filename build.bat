@echo off
setlocal EnableDelayedExpansion

cls

set SDK=css

if "%~1" == "install" (
    if not exist build mkdir build
    pushd build

    if not exist sdk_root (
        mkdir sdk_root
        pushd sdk_root

        git clone https://github.com/alliedmodders/sourcemod --recursive
        git clone https://github.com/alliedmodders/metamod-source
        git clone https://github.com/alliedmodders/hl2sdk hl2sdk-!SDK! -b !SDK!
        git clone https://github.com/alliedmodders/ambuild

        python -m venv venv
        call venv\Scripts\activate
        python -m pip install ./ambuild
        call deactivate

        popd
    )

    popd
) else if "%~1" == "build" (
    set ROOT_PATH=%~dp0
    set ROOT_PATH=!ROOT_PATH:~0,-1!
    if not exist build\output mkdir build\output

    call build\sdk_root\venv\Scripts\activate

    pushd build\output

    python ..\..\configure.py ^
        --sm-path=!ROOT_PATH!\build\sdk_root\sourcemod ^
        --mms-path=!ROOT_PATH!\build\sdk_root\metamod-source ^
        --hl2sdk-root=!ROOT_PATH!\build\sdk_root ^
        --hl2sdk-manifest-path=!ROOT_PATH!\build\sdk_root\sourcemod\hl2sdk-manifests ^
        --sdks=!SDK!

    ambuild

    deactivate

    popd
) else (
    echo No valid command was specified
)
