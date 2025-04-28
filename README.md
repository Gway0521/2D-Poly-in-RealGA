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
   cd 2D-Poly-in-RealGA/RealGA
   mkdir build
   cd build
   cmake ..
   cmake --build . --config Release
   ```

3. **Build 2D-Poly**

   ```
   cd ../../2D-Poly
   mkdir build
   cd build
   cmake ..
   cmake --build . --config Release
   ```

4. **Run the example**

   ```
   cd release

   ./triangulation_test

   ./realga_test [numNodes] [nInitial] [gen] [imagePath]
   ./realga_test 50 50 50 ../../dataset/01.jpg
   ```
