#include "triangulation.h"

#include <opencv2/opencv.hpp>

#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>

namespace triangulation
{

    bool Edge::operator==(const Edge &e) const
    {
        return (a == e.a && b == e.b) || (a == e.b && b == e.a);
    }

    // 計算三個點的有向面積（正值表示逆時針排列
    double ComputeDirectedArea(const cv::Point &a, const cv::Point &b, const cv::Point &c)
    {
        return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
    }

    // 判斷點 p 是否在由 (a, b, c) 構成三角形的外接圓內
    // 此函式要求 a, b, c 為逆時針排列，則 determinant > 0 表示 p 落在外接圓內
    bool IsInCircumcircle(const cv::Point &p, const cv::Point &a, const cv::Point &b, const cv::Point &c)
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

    TriangulationImageBuilder::TriangulationImageBuilder(int width, int height) : width_(width), height_(height) {};
    TriangulationImageBuilder::TriangulationImageBuilder(int width, int height, const std::vector<cv::Point> &points) : width_(width), height_(height)
    {
        RunDelaunay(points);
    };

    // 使用 Bowyer–Watson 演算法構造 Delaunay 三角剖分
    // 傳入的 points 陣列會暫時加入超大三角形的頂點，供演算法使用。
    // 為避免在最後輸出結果時混淆，我們約定原始點數為 original_n，超大三角形的頂點索引皆大於等於 original_n。
    void TriangulationImageBuilder::RunDelaunay(const std::vector<cv::Point> &points)
    {
        points_ = points;
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

        // ??芷?斗??敺?銝???????瑽????暺?
        points_.erase(points_.end() - 3, points_.end());
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

    void TriangulationImageBuilder::DrawColoredImage(const cv::Mat &orig_image, int mode)
    {
        colored_image_ = cv::Mat(height_, width_, CV_8UC3, cv::Scalar(255, 255, 255)); // 白底圖
        cv::Mat mask(height_, width_, CV_8UC1, cv::Scalar(0));

        auto quantize555 = [](const cv::Vec3b &px) -> cv::Vec3b
        {
            return {
                static_cast<uchar>(px[0] & 0xF8),
                static_cast<uchar>(px[1] & 0xF8),
                static_cast<uchar>(px[2] & 0xF8)};
        };

        for (const auto &tri : triangles_)
        {
            const cv::Point &a = points_[tri.a];
            const cv::Point &b = points_[tri.b];
            const cv::Point &c = points_[tri.c];

            std::vector<cv::Point> contour = {a, b, c};

            // 建立 mask，找出三角形內 pixel
            mask.setTo(0);
            cv::fillConvexPoly(mask, contour, 255);

            std::vector<cv::Point> nz;
            cv::findNonZero(mask, nz);

            cv::Scalar fill_color = cv::Scalar(0, 0, 0); // fallback default

            if (mode == 1)
            {
                // -------- Mode 1: 眾數 + 5-bit 量化 --------
                std::unordered_map<int, int> hist;

                auto hash555 = [](const cv::Vec3b &px) -> int
                {
                    return ((px[2] & 0xF8) << 10) | ((px[1] & 0xF8) << 5) | (px[0] & 0xF8);
                };

                for (const auto &p : nz)
                {
                    if (orig_image.channels() == 4)
                    {
                        const cv::Vec4b &px = orig_image.at<cv::Vec4b>(p);
                        if (px[3] == 0) // ignore transparent background
                            continue;
                        ++hist[hash555(cv::Vec3b(px[0], px[1], px[2]))];
                    }
                    else
                    {
                        const cv::Vec3b &px = orig_image.at<cv::Vec3b>(p);
                        ++hist[hash555(px)];
                    }
                }

                int max_freq = 0;
                int best_color = 0;

                for (const auto &kv : hist)
                {
                    if (kv.second > max_freq)
                    {
                        max_freq = kv.second;
                        best_color = kv.first;
                    }
                }

                uchar r = (best_color >> 10) & 0xF8;
                uchar g = (best_color >> 5) & 0xF8;
                uchar b = best_color & 0xF8;

                fill_color = cv::Scalar(b, g, r);
            }
            else if (mode == 2)
            {
                // -------- Mode 2: 眾數 (不量化，直接用 24-bit 原色) --------
                std::unordered_map<int, int> hist;

                auto rgb_to_key = [](const cv::Vec3b &px) -> int
                {
                    return (px[2] << 16) | (px[1] << 8) | px[0]; // R<<16 | G<<8 | B
                };

                for (const auto &p : nz)
                {
                    if (orig_image.channels() == 4)
                    {
                        const cv::Vec4b &px = orig_image.at<cv::Vec4b>(p);
                        if (px[3] == 0)
                            continue;
                        ++hist[rgb_to_key(cv::Vec3b(px[0], px[1], px[2]))];
                    }
                    else
                    {
                        const cv::Vec3b &px = orig_image.at<cv::Vec3b>(p);
                        ++hist[rgb_to_key(px)];
                    }
                }

                int max_freq = 0;
                int best_color = 0;

                for (const auto &kv : hist)
                {
                    if (kv.second > max_freq)
                    {
                        max_freq = kv.second;
                        best_color = kv.first;
                    }
                }

                uchar r = (best_color >> 16) & 0xFF;
                uchar g = (best_color >> 8) & 0xFF;
                uchar b = best_color & 0xFF;

                fill_color = cv::Scalar(b, g, r);
            }
            else
            {
                // -------- Mode 3: 平均色 --------
                cv::Vec3d acc(0, 0, 0);
                int count = 0;

                for (const auto &p : nz)
                {
                    if (orig_image.channels() == 4)
                    {
                        const cv::Vec4b &px = orig_image.at<cv::Vec4b>(p);
                        if (px[3] == 0)
                            continue;
                        acc += cv::Vec3d(px[0], px[1], px[2]);
                    }
                    else
                    {
                        const cv::Vec3b &px = orig_image.at<cv::Vec3b>(p);
                        acc += px;
                    }
                    ++count;
                }

                if (count > 0)
                {
                    cv::Vec3b avg_color(
                        static_cast<uchar>(acc[0] / count),
                        static_cast<uchar>(acc[1] / count),
                        static_cast<uchar>(acc[2] / count));

                    fill_color = cv::Scalar(avg_color[0], avg_color[1], avg_color[2]);
                }
            }

            cv::fillConvexPoly(colored_image_, contour, fill_color);
        }
    };

    void TriangulationImageBuilder::WriteLineImage(const std::string filename) const
    {
        cv::imwrite(filename, this->line_image_);
    };

    void TriangulationImageBuilder::WriteColoredImage(const std::string filename) const
    {
        cv::imwrite(filename, this->colored_image_);
    };

    // 目前暫時用 MSE
    double TriangulationImageBuilder::ComputeFitness(const cv::Mat &orig_image) const
    {

        CV_Assert(orig_image.size() == colored_image_.size());
        CV_Assert(orig_image.type() == colored_image_.type());

        // Difference
        cv::Mat diff;
        cv::absdiff(orig_image, colored_image_, diff);
        diff.convertTo(diff, CV_32F);

        // Square
        diff = diff.mul(diff);

        // Accumulate
        cv::Scalar s = cv::sum(diff);
        double mse = 0.0;
        for (int i = 0; i < diff.channels(); ++i)
            mse += s[i];

        // Average
        // std::cout << "channels:" << orig_image.channels()<<"\n";
        mse /= (orig_image.total() * orig_image.channels());

        return mse;
    }
} // namespace triangulation
