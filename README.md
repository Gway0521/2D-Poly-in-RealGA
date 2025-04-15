# 2D-Poly-in-RealGA

## Project Structure

- `RealGA/`: [RealGA](https://github.com/alenic/realGA)
- `2D-Poly/`: 我們的 code
  - `testRealGA.cpp`: 測試 RealGA 能不能順利跑

## Build - Windows

1. **Git clone**

   ```
   git clone https://github.com/Gway0521/2D-Poly-in-RealGA.git
   ```

2. **Build RealGA**

   ```
   cd RealGA
   mkdir build
   cd build
   cmake ..
   MSBuild.exe .\RealGA.sln /p:Configuration=Release
   ```

3. **Build 2D-Poly**
   ```
   cd ../../2D-Poly
   mkdir build
   cd build
   cmake ..
   MSBuild.exe .\2D-Poly.sln /p:Configuration=Release
   (或者 cmake --build . --config Release 也可以)
   ```
