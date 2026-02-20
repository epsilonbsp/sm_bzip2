# SourceMod Extension Example
Showcase of how to build [SourceMod Sample Extension](https://github.com/alliedmodders/sourcemod/tree/master/public/sample_ext) using [AMBuild](https://github.com/alliedmodders/ambuild)

Important notes:
* This is mostly intended for **Counter Strike: Source**, but maybe you can change SDK variable in scripts and it could work for other games
* Requirement list might be incomplete
## Requirements
### Windows
* Download and install [Visual Studio](https://visualstudio.microsoft.com/) (Desktop development with C++)
* Download and install [Git](https://git-scm.com/install/windows)
* Download and install [Python](https://www.python.org/downloads/)
### Linux
* Install Dependencies

      sudo apt install git clang python3 python-is-python3 python3-pip python3.12-venv
## Usage
    # Install SDK
    .\build.bat install

    # Builds extension and outputs it in build/output
    .\build.bat build
