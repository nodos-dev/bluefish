# Bluefish Modules for Nodos

This folder contains the Nodos modules for Bluefish hardware.

## Build Instructions
1. Download latest Nodos release from [nodos.dev](https://nodos.dev)
2. Clone the repository under Nodos workspace Module directory
```bash
git clone https://github.com/mediaz/bluefish.git --recurse-submodules Module/bf-modules
```
3. Generate project files from workspace root directory using CMake:
```bash
cmake -S ./Toolchain/CMake -B Build
```
4. Build the project:
```bash
cmake --build Build
```

