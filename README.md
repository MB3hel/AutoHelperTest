# AutoHelperTest

## Building
**CHANGE THE FILE PATH IN AutoTest/src/main.cpp**
- Install CMake build system (https://cmake.org/)
- Make sure you have a C++ compiler installed (MSVC, GCC, etc)
- Open powershell/cmd in folder
- Run the following commands
```
mkdir build
cd build
```
Visual Studio MSVC Compiler:
```
cmake .. -G "Visual Studio 15 2017"
```
MinGW GCC
```
cmake .. -G "MinGW Makefiles"
```
Build:
```
cmake --build .
```

Resulting exe will be in build/AutoTest/ (maybe in debug or release subdir with visual studio)