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

   ./RealGA_test -image_path <path> -expName <name> [-numNodes <int>] [-nInitial <int>] [-gen <int>]
   [-selectionType <string>] [-tournamentSize <int>] [-tournamentProb <float>]
   [-crossoverType <string>] [-BLXAlpha <float>] [-mutationType <string>] [-mutationRate <float>]
   ```

- 在執行 RealGA_test 時，有必要參數（必填）：\
  `-image_path <path>`：輸入影像的檔案路徑\
  `-expName <name>`：實驗名稱，作為結果輸出資料夾的名稱

- 其餘參數為選填參數（非必填），調整演算法的細節：\
  `-numNodes <int>`：node 數量（default：80）\
  `-nInitial <int>`：population size（default：100）\
  `-gen <int>`：generation 數（default：100）\
  `-selectionType <string>`：selection type（default：tournament）\
  `-tournamentSize <int>`：tournament size（default：2）\
  `-tournamentProb <float>`：tournament probability（default：1）\
  `-crossoverType <string>`：crossover type（default：BLX1p）\
  `-BLXAlpha <float>`：BLX alpha（default：0.1）\
  `-mutationType <string>`：mutation type（default：uniform）\
  `-mutationRate <float>`：mutation rate（default：0.0125）
