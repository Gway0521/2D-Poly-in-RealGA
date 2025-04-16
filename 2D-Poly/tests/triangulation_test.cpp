#include "triangulation.h"

#include <opencv2/opencv.hpp>

#include <fstream>
#include <vector>

int main()
{
    // 檔案路徑
    const std::string input_path = "../../dataset/01.txt";

    std::ifstream input_file(input_path);
    if (!input_file) {
        std::cerr << "Failed to open input file: " << input_path << std::endl;
        return 1;
    }

    // 前三個數字是 點的數量、影像寬度、影像高度
    int n, w, h;
    input_file >> n >> w >> h;

    // 後面 2n 個數字是 n 個座標點
    std::vector<cv::Point> points(n);
    for (int i = 0; i < n; i++)
        input_file >> points[i].x >> points[i].y;
    input_file.close();

    // 執行 Delaunay 三角剖分
    DTImage dtImg(w, h, points);

    dtImg.getNumTriangles();
    dtImg.printTriangles();
    dtImg.drawLine();
    dtImg.writeLineImg("LineImg01.jpg");

    return 0;
}