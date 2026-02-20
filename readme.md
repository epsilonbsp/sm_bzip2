# SourceMod Bzip2 Extension
Simple implementation of bzip2 extension for SourceMod, so you can easily compress and extract files

Important notes:
* This is mostly intended for **Counter Strike: Source**, but maybe you can change SDK variable in scripts and it could work for other games
* Requirement list might be incomplete

## Build Requirements
### Windows
* Download and install [Visual Studio](https://visualstudio.microsoft.com/) (Desktop development with C++)
* Download and install [Git](https://git-scm.com/install/windows)
* Download and install [Python](https://www.python.org/downloads/)

### Linux
* Install Dependencies

      sudo apt install git clang python3 python-is-python3 python3-pip python3.12-venv

## Building
```sh
# Clone SourceMod, Metamod, HL2SDK, and set up AMBuild
.\build.bat install # Windows
./build.sh install  # Linux

# Clone the bzip2 source
.\build.bat install_vendor # Windows
./build.sh install_vendor  # Linux

# Build the extension (output goes to build/output)
.\build.bat build # Windows
./build.sh build  # Linux
```

## Usage
Include `bzip2` in your plugin and use the two available natives:

```sourcepawn
#include <bzip2>

// Compress a file into a .bz2 archive
// blockSize is optional (1 = fastest, 9 = best compression), defaults to 9
BZ2_CompressFile("maps/bhop_bfur.bsp", "maps/bhop_bfur.bsp.bz2");
BZ2_CompressFile("maps/bhop_bfur.bsp", "maps/bhop_bfur.bsp.bz2", 5);

// Extract a .bz2 file
BZ2_ExtractFile("maps/bhop_bfur.bsp.bz2", "maps/bhop_bfur.bsp");
```
