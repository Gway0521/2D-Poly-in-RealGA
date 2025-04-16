# 2D-Poly-in-RealGA

## Requirements

- [OpenCV – 4.10.0](https://opencv.org/releases/)

## Project Structure

- `RealGA/`: [RealGA](https://github.com/alenic/realGA)
- `2D-Poly/`
  - `include/`: header files
  - `src/`: source code
  - `tests/`: unit test
  - `dataset/`
    - `01.jpg`: 企鵝圖片
    - `01.txt`: `01.jpg` 的座標輸入

## Getting Started - Windows

1. **Clone the repository**

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

4. **Run the example**

   ```
   cd release
   realga_test
   triangulation_test
   ```
