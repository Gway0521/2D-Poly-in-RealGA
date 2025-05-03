#include "triangulation.h"

#include <opencv2/opencv.hpp>

#include <fstream>
#include <iostream>
#include <vector>

#include "fitnessfunction.h"
#include "fitness.h"
#include "color.h"


int main()
{
    // 檔案路徑
    const std::string input_path = "../../dataset/03.txt";
    const std::string image_path = "../../dataset/01.jpg";

    std::ifstream input_file(input_path);
    if (!input_file)
    {
        std::cerr << "Failed to open input file: " << input_path << std::endl;
        return 1;
    }

    // 前三個數字是 點的數量、影像寬度、影像高度
    int n, width, height;
    input_file >> n >> width >> height;

    // 後面 2n 個數字是 n 個座標點
    std::vector<cv::Point> points(n);
    for (int i = 0; i < n; i++)
        input_file >> points[i].x >> points[i].y;
    input_file.close();

    cv::Mat resized;
    double scale = 0.5;

    // 讀入圖片
    cv::Mat orig_img = cv::imread(image_path, cv::IMREAD_COLOR);
    std::cout << "orig_image size: " << orig_img.cols << " x " << orig_img.rows << std::endl;

    if (orig_img.empty())
    {
        std::cerr << "Failed to open input file: " << image_path << std::endl;
        return -1;
    }

    // 執行 Delaunay 三角剖分
    triangulation::TriangulationImageBuilder triangulation_image_builder(orig_img, std::make_unique<color::QuantizedMeanFill>());
    triangulation_image_builder.RunDelaunay(points);

    std::cout << "Number of points: " << triangulation_image_builder.points().size() << std::endl;
    std::cout << "Image width: " << triangulation_image_builder.width() << ", height: " << triangulation_image_builder.height() << std::endl;
    std::cout << "Number of triangles: " << triangulation_image_builder.triangles().size() << std::endl;

    triangulation_image_builder.DrawLineImage();
    triangulation_image_builder.WriteLineImage("LineImage01.jpg");
    cv::resize(triangulation_image_builder.line_image(), resized, cv::Size(), scale, scale);
    cv::imshow("LineImage01", resized);
    cv::waitKey(0);

    // 上色
    unique_ptr<color::QuantizedMeanFill> umf = std::make_unique<color::QuantizedMeanFill>();
    triangulation_image_builder.DrawColoredImage(orig_img, umf.get());
    triangulation_image_builder.WriteColoredImage("ColoredImage01.jpg");
    cv::resize(triangulation_image_builder.colored_image(), resized, cv::Size(), scale, scale);
    cv::imshow("ColoredImage01", resized);
    cv::waitKey(0);

    // 計算 fitness
    FitnessFunction* mse = new fitness::MSE(orig_img, std::make_unique<color::QuantizedMeanFill>());
    FitnessFunction* psnr = new fitness::PSNR(orig_img, std::make_unique<color::QuantizedMeanFill>());
    FitnessFunction* ssim = new fitness::SSIM(orig_img, std::make_unique<color::QuantizedMeanFill>());

    RealChromosome chromosome;
    for (auto point : points) {
        chromosome.gene.push_back(point.x);
        chromosome.gene.push_back(point.y);
    }

    std::cout << "MSE Fitness: " << mse->eval(chromosome) << std::endl;
    std::cout << "PSNR Fitness: " << psnr->eval(chromosome) << std::endl;
    std::cout << "SSIM Fitness: " << ssim->eval(chromosome) << std::endl;

    delete mse;
    delete psnr;
    delete ssim;

    return 0;
}