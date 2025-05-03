#ifndef TRIANGULATION_H_
#define TRIANGULATION_H_

#include <opencv2/opencv.hpp>

#include "color.h"
#include "geometry.h"

#include <string>
#include <vector>

namespace triangulation
{

    struct Edge
    {
        int a, b;

        bool operator==(const Edge &e) const;
    };

    double ComputeDirectedArea(const cv::Point &a, const cv::Point &b, const cv::Point &c);
    bool IsInCircumcircle(const cv::Point &p, const cv::Point &a, const cv::Point &b, const cv::Point &c);

    // 一組 Delaunay Triangulation 結果
    class TriangulationImageBuilder
    {
    public:
        TriangulationImageBuilder();
        TriangulationImageBuilder(const cv::Mat& original_image, std::unique_ptr<color::ColorFill> color_fill);

        // 跑一遍 Delaunay Triangulation，存在 triangles_
        void RunDelaunay(const std::vector<cv::Point> &points);

        // 畫出線圖，存在 line_image_
        void DrawLineImage();
        // 畫出上色圖，存在 colored_image_
        void DrawColoredImage();
        void DrawColoredImage(const cv::Mat &orig_image, color::ColorFill* color_fill);
        // 把 line_image_ 寫入檔案
        void WriteLineImage(const std::string &filename) const;
        // 把 colored_image_ 寫入檔案
        void WriteColoredImage(const std::string &filename) const;

        int width() const { return width_; }
        int height() const { return height_; }
        const std::vector<cv::Point> &points() const { return points_; }
        const std::vector<Triangle> &triangles() const { return triangles_; }
        const cv::Mat &line_image() const { CV_Assert(!line_image_.empty()); return line_image_; }
        const cv::Mat &colored_image() const { CV_Assert(!colored_image_.empty()); return colored_image_; }
        const cv::Mat &original_image() const { CV_Assert(!original_image_.empty()); return original_image_; }

        void set_original_image(const cv::Mat& original_image) { original_image_ = original_image; }
        void set_width(int width) { width_ = width; }
        void set_height(int height) { height_ = height; }
        void set_color_fill(std::unique_ptr<color::ColorFill> color_fill) { color_fill_ = std::move(color_fill); }

    private:
        int width_;
        int height_;

        std::unique_ptr<color::ColorFill> color_fill_;

        std::vector<cv::Point> points_;
        std::vector<Triangle> triangles_;

        cv::Mat line_image_;
        cv::Mat colored_image_;
        cv::Mat original_image_;
    };

} // namespace triangulation

#endif
