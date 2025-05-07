#ifndef TRIANGULATION_H_
#define TRIANGULATION_H_

#include <opencv2/opencv.hpp>

#include "color.h"
#include "geometry.h"

#include <string>
#include <vector>
#include <memory>


namespace triangulation
{

    // 做 Delaunay Triangulation + 上色
    class TriangulationImageBuilder
    {
    private:
        struct Edge
        {
            int a, b;

            bool operator==(const Edge& e) const;
        };
        static double ComputeDirectedArea(const cv::Point& a, const cv::Point& b, const cv::Point& c);
        static bool IsInCircumcircle(const cv::Point& p, const cv::Point& a, const cv::Point& b, const cv::Point& c);

    public:
        TriangulationImageBuilder();
        TriangulationImageBuilder(std::unique_ptr<color::ColorFill> color_fill); // 在 decode 時使用
        TriangulationImageBuilder(const cv::Mat& original_image, std::unique_ptr<color::ColorFill> color_fill); // 在 encode 時使用

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

        // encode 成二進制檔案
        int Encode(const std::string& filename);
        int Encode(const std::vector<cv::Point>& points, const cv::Mat& orig_image, std::unique_ptr<color::ColorFill> color_fill, const std::string& filename);
        // decode 成 colored_image_
        int Decode(const std::string& filename);

        int width() const { return width_; }
        int height() const { return height_; }
        std::unique_ptr<color::ColorFill> clone_color_fill() const { return color_fill_->clone(); }
        const std::vector<cv::Point> &points() const { return points_; }
        const std::vector<Triangle> &triangles() const { return triangles_; }
        const cv::Mat &line_image() const { CV_Assert(!line_image_.empty()); return line_image_; }
        const cv::Mat &colored_image() const { CV_Assert(!colored_image_.empty()); return colored_image_; }
        const cv::Mat &original_image() const { CV_Assert(!original_image_.empty()); return original_image_; }

        void set_original_image(const cv::Mat& original_image) { CV_Assert(!original_image.empty()); original_image_ = original_image; width_ = original_image.cols; height_ = original_image.rows; }
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
