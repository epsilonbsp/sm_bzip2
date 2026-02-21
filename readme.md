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
Include `bzip2` in your plugin:

```sourcepawn
#include <bzip2>
```

### Synchronous

```sourcepawn
// Compress a file into a .bz2 archive
// blockSize is optional (1 = fastest, 9 = best compression), defaults to 9
BZ2Error err = BZ2_CompressFile("maps/bhop_bfur.bsp", "maps/bhop_bfur.bsp.bz2");
BZ2Error err = BZ2_CompressFile("maps/bhop_bfur.bsp", "maps/bhop_bfur.bsp.bz2", 5);

// Decompress a .bz2 file
BZ2Error err = BZ2_DecompressFile("maps/bhop_bfur.bsp.bz2", "maps/bhop_bfur.bsp");
```

### Asynchronous

The async variants run on a background thread and invoke a callback on the main thread when done:

```sourcepawn
void OnDecompressDone(BZ2Error error, const char[] src, const char[] dest, any data)
{
    if (error == BZ2_OK)
        PrintToServer("Decompressed %s -> %s", src, dest);
    else
        PrintToServer("Decompress failed: %d", error);
}

void OnCompressDone(BZ2Error error, const char[] src, const char[] dest, any data)
{
    if (error == BZ2_OK)
        PrintToServer("Compressed %s -> %s", src, dest);
}

// Decompress asynchronously
BZ2_DecompressFileAsync("maps/bhop_bfur.bsp.bz2", "maps/bhop_bfur.bsp", OnDecompressDone);

// Compress asynchronously (blockSize is optional, defaults to 9)
BZ2_CompressFileAsync("maps/bhop_bfur.bsp", "maps/bhop_bfur.bsp.bz2", OnCompressDone);
BZ2_CompressFileAsync("maps/bhop_bfur.bsp", "maps/bhop_bfur.bsp.bz2", OnCompressDone, .blockSize=5);
```

### Error handling

Both sync and async natives return/pass a `BZ2Error` value:

| Value | Constant | Meaning |
|-------|----------|---------|
| 0 | `BZ2_OK` | Success |
| 1 | `BZ2_ERR_SRC_OPEN` | Failed to open source file |
| 2 | `BZ2_ERR_DEST_OPEN` | Failed to open destination file |
| 3 | `BZ2_ERR_MEM` | Memory allocation failure |
| 4 | `BZ2_ERR_DATA` | Data integrity (CRC) error |
| 5 | `BZ2_ERR_DATA_MAGIC` | Not a valid bzip2 stream |
| 6 | `BZ2_ERR_IO` | I/O error during read/write |
| 7 | `BZ2_ERR_UNEXPECTED_EOF` | Truncated bzip2 stream |
| 8 | `BZ2_ERR_WRITE` | Failed to write output file |
| 9 | `BZ2_ERR_UNKNOWN` | Other unexpected error |
