#ifndef TRIANGULATION_H_
#define TRIANGULATION_H_

#include <opencv2/opencv.hpp>

#include <string>
#include <vector>

namespace triangulation {

    struct Triangle {
        int a, b, c;
    };

    struct Edge {
        int a, b;

        bool operator==(const Edge& e) const;
    };

    double ComputeDirectedArea(const cv::Point& a, const cv::Point& b, const cv::Point& c);
    bool IsInCircumcircle(const cv::Point& p, const cv::Point& a, const cv::Point& b, const cv::Point& c);

    // 一組 Delaunay Triangulation 結果
    class TriangulationImageBuilder {
    public:
        TriangulationImageBuilder(int w, int h);
        TriangulationImageBuilder(int w, int h, const std::vector<cv::Point>& points);

        // 跑一遍 Delaunay Triangulation，存在 triangles_
        void RunDelaunay(const std::vector<cv::Point>& points);

        // 畫出線圖，存在 line_image_
        void DrawLineImage();
        // 畫出上色圖，存在 colored_image_
        void DrawColoredImage(const cv::Mat& orig_image);
        // 把 line_image_ 寫入檔案
        void WriteLineImage(const std::string filename) const;
        // 把 colored_image_ 寫入檔案
        void WriteColoredImage(const std::string filename) const;

        // 計算 fitness
        double ComputeFitness(const cv::Mat& orig_image) const;
        
        int width() const { return width_; }
        int height() const { return height_; }
        const std::vector<cv::Point>& points() const { return points_; }
        const std::vector<Triangle>& triangles() const { return triangles_; }
        const cv::Mat& line_image() const { return line_image_; }
        const cv::Mat& colored_image() const { return colored_image_; }

    private:
        int width_;
        int height_;

        std::vector<cv::Point> points_;
        std::vector<Triangle> triangles_;

        cv::Mat line_image_;
        cv::Mat colored_image_;
    };

} // namespace triangulation

#endif
