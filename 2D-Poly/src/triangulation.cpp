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

    TriangulationImageBuilder::TriangulationImageBuilder() { width_ = -1; height_ = -1; };
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

    void TriangulationImageBuilder::DrawColoredImage(const cv::Mat& orig_image, int mode)
    {
        // 準備 output 與 mask
        colored_image_.create(height_, width_, CV_8UC3);
        colored_image_.setTo(cv::Scalar(255, 255, 255));  // 白底
        cv::Mat mask(height_, width_, CV_8UC1, cv::Scalar(0));

        // 針對 mode 1，預先建立 5-bit histogram 與鍵集合
        static std::vector<int> hist5bit(32 * 32 * 32);
        std::vector<int> keys5bit;
        keys5bit.reserve(1024);

        // lambda for hash555
        auto hash555 = [](const cv::Vec3b& px)->int {
            return ((px[2] & 0xF8) >> 3) * 32 * 32
                + ((px[1] & 0xF8) >> 3) * 32
                + ((px[0] & 0xF8) >> 3);
            };

        // 遍歷所有三角形
        for (const auto& tri : triangles_)
        {
            // 取出三個頂點，算出 bounding box
            cv::Point pts[3] = {
                points_[tri.a],
                points_[tri.b],
                points_[tri.c]
            };
            cv::Rect roi = cv::boundingRect(std::vector<cv::Point>(pts, pts + 3))
                & cv::Rect(0, 0, width_, height_);

            // 只在 ROI 內清除舊 mask、並填新三角形
            auto maskROI = mask(roi);
            maskROI.setTo(0);
            cv::Point triROI[3] = {
                pts[0] - roi.tl(),
                pts[1] - roi.tl(),
                pts[2] - roi.tl()
            };
            cv::fillConvexPoly(maskROI, triROI, 3, cv::Scalar(255), cv::LINE_AA);

            // 根據 mode 統計顏色
            cv::Scalar fill_color(0, 0, 0);
            if (mode == 1)
            {
                // 重置 histogram（只清用到的）
                for (int idx : keys5bit) hist5bit[idx] = 0;
                keys5bit.clear();

                int max_freq = 0, best_idx = 0;
                for (int y = roi.y; y < roi.y + roi.height; ++y)
                {
                    const uchar* mrow = mask.ptr<uchar>(y) + roi.x;
                    for (int x = roi.x; x < roi.x + roi.width; ++x, ++mrow)
                    {
                        if (!*mrow) continue;
                        cv::Vec3b px = orig_image.at<cv::Vec3b>(y, x);
                        int idx = hash555(px);
                        if (hist5bit[idx]++ == 0) keys5bit.push_back(idx);
                        if (hist5bit[idx] > max_freq) {
                            max_freq = hist5bit[idx];
                            best_idx = idx;
                        }
                    }
                }
                // 解出 r/g/b
                uchar r5 = (best_idx / (32 * 32)) << 3;
                uchar g5 = ((best_idx / 32) % 32) << 3;
                uchar b5 = (best_idx % 32) << 3;
                fill_color = cv::Scalar(b5, g5, r5);
            }
            else if (mode == 2)
            {
                // 24-bit 原色 histogram
                static thread_local std::unordered_map<int, int> hist24bit;
                hist24bit.clear();
                hist24bit.reserve(roi.area() / 4);  // 估計大概大小
                int max_freq = 0, best_key = 0;

                for (int y = roi.y; y < roi.y + roi.height; ++y)
                {
                    const uchar* mrow = mask.ptr<uchar>(y) + roi.x;
                    for (int x = roi.x; x < roi.x + roi.width; ++x, ++mrow)
                    {
                        if (!*mrow) continue;
                        cv::Vec3b px = orig_image.at<cv::Vec3b>(y, x);
                        int key = (px[2] << 16) | (px[1] << 8) | px[0];
                        int freq = ++hist24bit[key];
                        if (freq > max_freq) {
                            max_freq = freq;
                            best_key = key;
                        }
                    }
                }
                fill_color = cv::Scalar((best_key & 0xFF),
                    (best_key >> 8) & 0xFF,
                    (best_key >> 16) & 0xFF);
            }
            else  // mode 3
            {
                cv::Vec3d acc(0, 0, 0);
                int cnt = 0;
                for (int y = roi.y; y < roi.y + roi.height; ++y)
                {
                    const uchar* mrow = mask.ptr<uchar>(y) + roi.x;
                    for (int x = roi.x; x < roi.x + roi.width; ++x, ++mrow)
                    {
                        if (!*mrow) continue;
                        cv::Vec3b px = orig_image.at<cv::Vec3b>(y, x);
                        acc += px;
                        ++cnt;
                    }
                }
                if (cnt > 0) {
                    acc /= cnt;
                    fill_color = cv::Scalar((uchar)acc[0],
                        (uchar)acc[1],
                        (uchar)acc[2]);
                }
            }

            // 在 output 圖上填色
            cv::fillConvexPoly(
                colored_image_,
                pts, 3,
                fill_color,
                cv::LINE_AA
            );
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
