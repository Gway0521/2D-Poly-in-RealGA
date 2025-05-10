#include "triangulation.h"

#include <opencv2/opencv.hpp>

#include "color.h"
#include "geometry.h"

#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>
#include <fstream>


namespace triangulation
{

    bool TriangulationImageBuilder::Edge::operator==(const Edge &e) const
    {
        return (a == e.a && b == e.b) || (a == e.b && b == e.a);
    }

    // 計算三個點的有向面積（正值表示逆時針排列
    double TriangulationImageBuilder::ComputeDirectedArea(const cv::Point &a, const cv::Point &b, const cv::Point &c)
    {
        return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
    }

    // 判斷點 p 是否在由 (a, b, c) 構成三角形的外接圓內
    // 此函式要求 a, b, c 為逆時針排列，則 determinant > 0 表示 p 落在外接圓內
    bool TriangulationImageBuilder::IsInCircumcircle(const cv::Point &p, const cv::Point &a, const cv::Point &b, const cv::Point &c)
    {
        double ax = a.x - p.x;
        double ay = a.y - p.y;
        double bx = b.x - p.x;
        double by = b.y - p.y;
        double cx = c.x - p.x;
        double cy = c.y - p.y;

        double det = (ax * ax + ay * ay) * (bx * cy - cx * by) -
                     (bx * bx + by * by) * (ax * cy - cx * ay) +
                     (cx * cx + cy * cy) * (ax * by - bx * ay);
        return det > 0;
    }

    TriangulationImageBuilder::TriangulationImageBuilder() :
        width_(-1), height_(-1), color_fill_(new color::BarycentricFill()) {}
    TriangulationImageBuilder::TriangulationImageBuilder(std::unique_ptr<color::ColorFill> color_fill) :
        width_(-1), height_(-1), color_fill_(std::move(color_fill)) {}
    TriangulationImageBuilder::TriangulationImageBuilder(const cv::Mat& original_image, std::unique_ptr<color::ColorFill> color_fill):
        original_image_(original_image), color_fill_(std::move(color_fill)), width_(original_image.cols), height_(original_image.rows) {}

    void TriangulationImageBuilder::RunEdgeDetection(float top_p) {
        DrawEdgeImage();

        std::vector<std::pair<uchar, cv::Point>> edge_points;
        for (int y = 0; y < edge_image_.rows; ++y) {
            const uchar* row = edge_image_.ptr<uchar>(y);
            for (int x = 0; x < edge_image_.cols; ++x) {
                if (row[x] > 0)
                    edge_points.emplace_back(row[x], cv::Point(x, y));
            }
        }

        std::sort(edge_points.begin(), edge_points.end(), [](const auto& a, const auto& b) {
            return a.first > b.first;
            });

        size_t keep = static_cast<size_t>(edge_points.size() * top_p);
        gene_to_point_.clear();
        gene_to_point_.reserve(keep);
        for (size_t i = 0; i < keep; ++i)
            gene_to_point_.push_back(edge_points[i].second);
    }

    // 使用 Bowyer–Watson 演算法構造 Delaunay 三角剖分
    // 傳入的 points 陣列會暫時加入超大三角形的頂點，供演算法使用
    // 為避免在最後輸出結果時混淆，我們約定原始點數為 original_n，超大三角形的頂點索引皆大於等於 original_n
    void TriangulationImageBuilder::RunDelaunay(const std::vector<cv::Point> &points)
    {
        points_ = points;
        triangles_.clear();
        int original_n = points_.size();

        // 計算點集合的邊界
        double min_x = points_[0].x, min_y = points_[0].y;
        double max_x = points_[0].x, max_y = points_[0].y;
        for (const auto &p : points_)
        {
            if (p.x < min_x)
                min_x = p.x;
            if (p.x > max_x)
                max_x = p.x;
            if (p.y < min_y)
                min_y = p.y;
            if (p.y > max_y)
                max_y = p.y;
        }
        double dx = max_x - min_x, dy = max_y - min_y;
        double delta_max = std::max(dx, dy);
        double mid_x = (min_x + max_x) / 2.0;
        double mid_y = (min_y + max_y) / 2.0;

        // 建立一個足夠大的超大三角形，保證所有輸入點都包含在內
        cv::Point p1 = {int(mid_x - 2 * delta_max), int(mid_y - delta_max)};
        cv::Point p2 = {int(mid_x), int(mid_y + 2 * delta_max)};
        cv::Point p3 = {int(mid_x + 2 * delta_max), int(mid_y - delta_max)};

        // 將超大三角形的頂點加入點集合中
        points_.push_back(p1);
        points_.push_back(p2);
        points_.push_back(p3);
        int idx_p1 = points_.size() - 3;
        int idx_p2 = points_.size() - 2;
        int idx_p3 = points_.size() - 1;

        // 初始三角形即為超大三角形
        triangles_.push_back({idx_p1, idx_p2, idx_p3});

        // 將每個原始點依序插入
        // 注意：這邊只迭代原始點（索引 0 ~ original_n-1）
        for (int i = 0; i < original_n; i++)
        {
            cv::Point p = points_[i];
            std::vector<Triangle> bad_triangles; // 儲存外接圓包含 p 的三角形
            std::vector<Edge> polygon;           // 儲存多邊形邊界

            // 找出所有外接圓包含 p 的三角形
            for (int j = 0; j < triangles_.size(); j++)
            {
                Triangle tri = triangles_[j];
                cv::Point a = points_[tri.a];
                cv::Point b = points_[tri.b];
                cv::Point c = points_[tri.c];
                // 若三角形不是逆時針排列則調整之
                if (ComputeDirectedArea(a, b, c) < 0)
                    swap(b, c);
                if (IsInCircumcircle(p, a, b, c))
                    bad_triangles.push_back(tri);
            }

            // 從所有不適合的三角形中找到「多邊形孔」的邊界
            // 方法：對每個不適合三角形的每一邊，如果該邊在其他不適合三角形中沒有出現過，則此邊屬於邊界
            for (size_t i_tri = 0; i_tri < bad_triangles.size(); i_tri++)
            {
                // 三角形有三個邊
                Edge edges[3] = {
                    {bad_triangles[i_tri].a, bad_triangles[i_tri].b},
                    {bad_triangles[i_tri].b, bad_triangles[i_tri].c},
                    {bad_triangles[i_tri].c, bad_triangles[i_tri].a}};
                for (int e = 0; e < 3; e++)
                {
                    bool shared = false;
                    // 與其他不適合三角形的邊做比對
                    for (size_t j_tri = 0; j_tri < bad_triangles.size(); j_tri++)
                    {
                        if (i_tri == j_tri)
                            continue;
                        Edge other_edges[3] = {
                            {bad_triangles[j_tri].a, bad_triangles[j_tri].b},
                            {bad_triangles[j_tri].b, bad_triangles[j_tri].c},
                            {bad_triangles[j_tri].c, bad_triangles[j_tri].a}};
                        for (int k = 0; k < 3; k++)
                        {
                            if (edges[e] == other_edges[k])
                            {
                                shared = true;
                                break;
                            }
                        }
                        if (shared)
                            break;
                    }
                    if (!shared)
                        polygon.push_back(edges[e]);
                }
            }

            // 刪除所有外接圓包含 p 的三角形
            triangles_.erase(remove_if(triangles_.begin(), triangles_.end(),
                                       [p, this](const Triangle &tri)
                                       {
                                           cv::Point a = this->points_[tri.a];
                                           cv::Point b = this->points_[tri.b];
                                           cv::Point c = this->points_[tri.c];
                                           if (ComputeDirectedArea(a, b, c) < 0)
                                               swap(b, c);
                                           return IsInCircumcircle(p, a, b, c);
                                       }),
                             triangles_.end());

            // 將新點 p 與多邊形邊界各邊連結形成新的三角形
            for (auto &edge : polygon)
                triangles_.push_back({edge.a, edge.b, i});
        }

        // 刪除所有三角形中出現超大三角形頂點的那些（這些三角形是虛構出來的輔助部份）
        triangles_.erase(remove_if(triangles_.begin(), triangles_.end(),
                                   [original_n](const Triangle &tri)
                                   {
                                       return (tri.a >= original_n || tri.b >= original_n || tri.c >= original_n);
                                   }),
                         triangles_.end());

        points_.erase(points_.end() - 3, points_.end());
    }

    void TriangulationImageBuilder::DrawEdgeImage()
    {
        std::vector<cv::Mat> channels;
        cv::split(original_image_, channels);

        cv::Mat gradX, gradY, gradMag, maxGrad = cv::Mat::zeros(original_image_.size(), CV_32F);

        for (int i = 0; i < 3; ++i) {
            cv::Sobel(channels[i], gradX, CV_32F, 1, 0, 3);
            cv::Sobel(channels[i], gradY, CV_32F, 0, 1, 3);
            cv::magnitude(gradX, gradY, gradMag);
            cv::max(maxGrad, gradMag, maxGrad);
        }

        // Normalize to [0, 1]
        cv::Mat gradNormalized;
        cv::normalize(maxGrad, gradNormalized, 0.0, 1.0, cv::NORM_MINMAX);

        // Apply gamma correction to boost dark areas
        const float gamma = 0.6f;
        cv::Mat gradGamma;
        cv::pow(gradNormalized, gamma, gradGamma);

        gradGamma *= 255.0f;
        gradGamma.convertTo(edge_image_, CV_8U);
    }

    void TriangulationImageBuilder::DrawLineImage()
    {
        line_image_ = cv::Mat(height_, width_, CV_8UC3, cv::Scalar(255, 255, 255));

        for (const auto &tri : triangles_)
        {
            cv::line(line_image_, points_[tri.a], points_[tri.b], cv::Scalar(0, 0, 0), 1);
            cv::line(line_image_, points_[tri.a], points_[tri.c], cv::Scalar(0, 0, 0), 1);
            cv::line(line_image_, points_[tri.b], points_[tri.c], cv::Scalar(0, 0, 0), 1);
        }
    }

    void TriangulationImageBuilder::DrawColoredImage() { 
        DrawColoredImage(original_image_, color_fill_.get()); 
    }

    void TriangulationImageBuilder::DrawColoredImage(const cv::Mat& orig_image, color::ColorFill* color_fill)
    {
        CV_Assert(!orig_image.empty());

        colored_image_ = color_fill->Draw(orig_image, triangles_, points_);
    }

    void TriangulationImageBuilder::WriteLineImage(const std::string& filename) const
    {
        CV_Assert(!line_image_.empty());
        cv::imwrite(filename, this->line_image_);
    }

    void TriangulationImageBuilder::WriteColoredImage(const std::string& filename) const
    {
        CV_Assert(!colored_image_.empty());
        cv::imwrite(filename, this->colored_image_);
    }

    int TriangulationImageBuilder::Encode(const std::string& filename)
    {
        CV_Assert(!original_image_.empty());
        std::vector<cv::Vec3b> encoded_colors = color_fill_->EncodeColor(original_image_, triangles_, points_);

        std::ofstream ofs(filename, std::ios::binary);
        if (!ofs.is_open()) {
            std::cerr << "Failed to open file for writing: " << filename << std::endl;
            return -1;
        }

        // 把圖片 height, width 寫入檔案
        ofs.write(reinterpret_cast<const char*>(&height_), sizeof(int32_t));
        ofs.write(reinterpret_cast<const char*>(&width_), sizeof(int32_t));

        // 把 points 寫入檔案
        int32_t num_points = static_cast<int32_t>(points_.size());
        ofs.write(reinterpret_cast<const char*>(&num_points), sizeof(int32_t));
        for (const auto& pt : points_) {
            int32_t x = pt.x;
            int32_t y = pt.y;
            ofs.write(reinterpret_cast<const char*>(&x), sizeof(int32_t));
            ofs.write(reinterpret_cast<const char*>(&y), sizeof(int32_t));
        }

        // 把 colors 寫入檔案
        int32_t num_colors = static_cast<int32_t>(encoded_colors.size());
        ofs.write(reinterpret_cast<const char*>(&num_colors), sizeof(int32_t));
        for (const auto& color : encoded_colors) {
            ofs.write(reinterpret_cast<const char*>(&color[0]), sizeof(uchar));  // B
            ofs.write(reinterpret_cast<const char*>(&color[1]), sizeof(uchar));  // G
            ofs.write(reinterpret_cast<const char*>(&color[2]), sizeof(uchar));  // R
        }

        ofs.close();

        // 回傳檔案大小
        std::ifstream ifs(filename, std::ios::binary | std::ios::ate);
        return static_cast<int>(ifs.tellg());
    }

    int TriangulationImageBuilder::Encode(const std::vector<cv::Point>& points, const cv::Mat& orig_image, std::unique_ptr<color::ColorFill> color_fill, const std::string& filename)
    {
        CV_Assert(!orig_image.empty());
        original_image_ = orig_image;
        color_fill_ = std::move(color_fill);

        RunDelaunay(points);
        return Encode(filename);
    }

    int TriangulationImageBuilder::Decode(const std::string& filename)
    {
        std::ifstream ifs(filename, std::ios::binary | std::ios::ate);
        if (!ifs.is_open()) {
            std::cerr << "Failed to open file for reading: " << filename << std::endl;
            return -1;
        }

        std::streamsize file_size = ifs.tellg();
        ifs.seekg(0, std::ios::beg);

        // 讀取 height, width
        ifs.read(reinterpret_cast<char*>(&height_), sizeof(int32_t));
        ifs.read(reinterpret_cast<char*>(&width_), sizeof(int32_t));

        // 把 points 讀入
        int32_t num_points = 0;
        ifs.read(reinterpret_cast<char*>(&num_points), sizeof(int32_t));
        points_.resize(num_points);
        ifs.read(reinterpret_cast<char*>(points_.data()), num_points * sizeof(cv::Point));

        // 把 colors 讀入
        int32_t num_colors = 0;
        ifs.read(reinterpret_cast<char*>(&num_colors), sizeof(int32_t));
        std::vector<cv::Vec3b> colors(num_colors);
        ifs.read(reinterpret_cast<char*>(colors.data()), num_colors * sizeof(cv::Vec3b));

        RunDelaunay(points_);
        colored_image_ = color_fill_->Draw(height_, width_, colors, triangles_, points_);

        // 回傳檔案大小
        return static_cast<int>(file_size);
    }

} // namespace triangulation
