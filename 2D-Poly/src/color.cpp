#include "color.h"

#include <opencv2/opencv.hpp>

#include "geometry.h"

#include <memory>
#include <vector>
#include <cmath>
#include <algorithm>
#include <unordered_map>


std::string ToString(ColorFillMode mode) {
    if (mode == ColorFillMode::kMean) return "Mean (mode 1)";
    else if (mode == ColorFillMode::kQuantizedMean) return "Quantized Mean (mode 2)";
    else if (mode == ColorFillMode::kMajority) return "Majority (mode 3)";
    else if (mode == ColorFillMode::kBarycentric) return "Barycentric (mode 4)";
    else return "Unknown (Unknown mode)";
}

namespace color
{
    std::unique_ptr<ColorFill> BarycentricFill::clone() const {
        return std::make_unique<BarycentricFill>(*this);
    }
    std::unique_ptr<ColorFill> MajorityFill::clone() const {
        return std::make_unique<MajorityFill>(*this);
    }
    std::unique_ptr<ColorFill> MeanFill::clone() const {
        return std::make_unique<MeanFill>(*this);
    }
    std::unique_ptr<ColorFill> QuantizedMeanFill::clone() const {
        return std::make_unique<QuantizedMeanFill>(*this);
    }

    std::vector<cv::Vec3b> BarycentricFill::EncodeColor(const cv::Mat& orig_img, const std::vector<Triangle>& triangles, const std::vector<cv::Point>& points) const {
        const int n = points.size();
        std::vector<cv::Vec3b> encoded_color(n);

        for (int i = 0; i < n; ++i)
            encoded_color[i] = orig_img.at<cv::Vec3b>(points[i]);
        return encoded_color;
    }

	cv::Mat BarycentricFill::Draw(const cv::Mat& orig_img, const std::vector<Triangle>& triangles, const std::vector<cv::Point>& points) const {

        const int width = orig_img.cols;
        const int height = orig_img.rows;
        cv::Mat colored_image(height, width, CV_8UC3, cv::Scalar(255, 255, 255));

        std::vector<cv::Vec3b> colors = EncodeColor(orig_img, triangles, points);

        // 用 cv::Point2f 是為了後續的 cv::Matx22f，cv::Vec3f 是為了線性插值
        for (const auto& tri : triangles) {
            const cv::Point2f p0 = points[tri.a];
            const cv::Point2f p1 = points[tri.b];
            const cv::Point2f p2 = points[tri.c];
            const cv::Vec3f c0 = colors[tri.a];
            const cv::Vec3f c1 = colors[tri.b];
            const cv::Vec3f c2 = colors[tri.c];

            // Bounding box
            cv::Rect roi = cv::boundingRect(std::vector<cv::Point>{cv::Point(p0), cv::Point(p1), cv::Point(p2)})& cv::Rect(0, 0, width, height);

            // 建立 2×2 矩陣 M = [ p0-p2, p1-p2 ], 並反矩陣一次
            cv::Matx22f M(p0.x - p2.x, p1.x - p2.x,
                p0.y - p2.y, p1.y - p2.y);
            cv::Matx22f M_inv = M.inv();

            // 填色
            for (int y = roi.y; y < roi.y + roi.height; ++y) {
                cv::Vec3b* dst_row = colored_image.ptr<cv::Vec3b>(y);
                for (int x = roi.x; x < roi.x + roi.width; ++x) {
                    // 計算相對 p2 的向量 v = [x-p2.x, y-p2.y]
                    cv::Point2f v(x - p2.x, y - p2.y);

                    // 由 v 得到 (α, β)： [α; β] = M_inv * v
                    cv::Vec2f ab = M_inv * v;
                    float alpha = ab[0];
                    float beta = ab[1];
                    float gamma = 1.0f - alpha - beta;

                    // 只有在三個重心係數都 >= 0 時才表示在三角形內
                    if (alpha >= 0 && beta >= 0 && gamma >= 0) {
                        // 線性插值顏色
                        cv::Vec3f c = alpha * c0 + beta * c1 + gamma * c2;
                        // 存回 unsigned char
                        dst_row[x] = cv::Vec3b(
                            static_cast<uchar>(c[0] + 0.5f),
                            static_cast<uchar>(c[1] + 0.5f),
                            static_cast<uchar>(c[2] + 0.5f)
                        );
                    }
                }
            }
        }

        return colored_image;
	}

    cv::Mat BarycentricFill::Draw(int height, int width, const std::vector<cv::Vec3b>& colors, const std::vector<Triangle>& triangles, const std::vector<cv::Point>& points) const {

        cv::Mat colored_image(height, width, CV_8UC3, cv::Scalar(255, 255, 255));

        // 用 cv::Point2f 是為了後續的 cv::Matx22f，cv::Vec3f 是為了線性插值
        for (const auto& tri : triangles) {
            const cv::Point2f p0 = points[tri.a];
            const cv::Point2f p1 = points[tri.b];
            const cv::Point2f p2 = points[tri.c];
            const cv::Vec3f c0 = colors[tri.a];
            const cv::Vec3f c1 = colors[tri.b];
            const cv::Vec3f c2 = colors[tri.c];

            // Bounding box
            cv::Rect roi = cv::boundingRect(std::vector<cv::Point>{cv::Point(p0), cv::Point(p1), cv::Point(p2)})& cv::Rect(0, 0, width, height);

            // 建立 2×2 矩陣 M = [ p0-p2, p1-p2 ], 並反矩陣一次
            cv::Matx22f M(p0.x - p2.x, p1.x - p2.x,
                p0.y - p2.y, p1.y - p2.y);
            cv::Matx22f M_inv = M.inv();

            // 填色
            for (int y = roi.y; y < roi.y + roi.height; ++y) {
                cv::Vec3b* dst_row = colored_image.ptr<cv::Vec3b>(y);
                for (int x = roi.x; x < roi.x + roi.width; ++x) {
                    // 計算相對 p2 的向量 v = [x-p2.x, y-p2.y]
                    cv::Point2f v(x - p2.x, y - p2.y);

                    // 由 v 得到 (α, β)： [α; β] = M_inv * v
                    cv::Vec2f ab = M_inv * v;
                    float alpha = ab[0];
                    float beta = ab[1];
                    float gamma = 1.0f - alpha - beta;

                    // 只有在三個重心係數都 >= 0 時才表示在三角形內
                    if (alpha >= 0 && beta >= 0 && gamma >= 0) {
                        // 線性插值顏色
                        cv::Vec3f c = alpha * c0 + beta * c1 + gamma * c2;
                        // 存回 unsigned char
                        dst_row[x] = cv::Vec3b(
                            static_cast<uchar>(c[0] + 0.5f),
                            static_cast<uchar>(c[1] + 0.5f),
                            static_cast<uchar>(c[2] + 0.5f)
                        );
                    }
                }
            }
        }

        return colored_image;
    }

    std::vector<cv::Vec3b> MajorityFill::EncodeColor(const cv::Mat& orig_img, const std::vector<Triangle>& triangles, const std::vector<cv::Point>& points) const {

        const int width = orig_img.cols;
        const int height = orig_img.rows;
        std::vector<cv::Vec3b> encoded_colors;
        encoded_colors.reserve(triangles.size());

        cv::Mat mask(height, width, CV_8UC1, cv::Scalar(0));

        for (const auto& tri : triangles) {
            cv::Point pts[3] = {
                points[tri.a],
                points[tri.b],
                points[tri.c]
            };
            cv::Rect roi = cv::boundingRect(std::vector<cv::Point>(pts, pts + 3))
                & cv::Rect(0, 0, width, height);

            auto mroi = mask(roi);
            mroi.setTo(0);

            cv::Point triROI[3] = {
                pts[0] - roi.tl(),
                pts[1] - roi.tl(),
                pts[2] - roi.tl()
            };
            cv::fillConvexPoly(mroi, triROI, 3, cv::Scalar(255), cv::LINE_AA);

            std::unordered_map<int, int> hist;
            hist.reserve(roi.area() / 4);
            int bestKey = 0, bestFreq = 0;

            for (int y = roi.y; y < roi.y + roi.height; ++y) {
                const uchar* mrow = mask.ptr<uchar>(y) + roi.x;
                for (int x = roi.x; x < roi.x + roi.width; ++x, ++mrow) {
                    if (!*mrow) continue;
                    cv::Vec3b px = orig_img.at<cv::Vec3b>(y, x);
                    int key = (px[2] << 16) | (px[1] << 8) | px[0];
                    int freq = ++hist[key];
                    if (freq > bestFreq) {
                        bestFreq = freq;
                        bestKey = key;
                    }
                }
            }

            cv::Vec3b fill(
                static_cast<uchar>(bestKey & 0xFF),
                static_cast<uchar>((bestKey >> 8) & 0xFF),
                static_cast<uchar>((bestKey >> 16) & 0xFF)
            );
            encoded_colors.push_back(fill);
        }

        return encoded_colors;
    }

    cv::Mat MajorityFill::Draw(const cv::Mat& orig_img, const std::vector<Triangle>& triangles, const std::vector<cv::Point>& points) const {

        const int width = orig_img.cols;
        const int height = orig_img.rows;
        cv::Mat colored_image(height, width, CV_8UC3, cv::Scalar(255, 255, 255));

        std::vector<cv::Vec3b> colors = EncodeColor(orig_img, triangles, points);

        for (size_t i = 0; i < triangles.size(); ++i) {
            const auto& tri = triangles[i];
            const cv::Vec3b& fill_color = colors[i];
            const cv::Scalar fill(fill_color[0], fill_color[1], fill_color[2]);

            cv::Point pts[3] = {
                points[tri.a],
                points[tri.b],
                points[tri.c]
            };
            cv::fillConvexPoly(colored_image, pts, 3, fill, cv::LINE_AA);
        }

        return colored_image;
    }

    cv::Mat MajorityFill::Draw(int height, int width, const std::vector<cv::Vec3b>& colors, const std::vector<Triangle>& triangles, const std::vector<cv::Point>& points) const {

        cv::Mat colored_image(height, width, CV_8UC3, cv::Scalar(255, 255, 255));

        for (size_t i = 0; i < triangles.size(); ++i) {
            const auto& tri = triangles[i];
            const cv::Vec3b& fill_color = colors[i];
            const cv::Scalar fill(fill_color[0], fill_color[1], fill_color[2]);

            cv::Point pts[3] = {
                points[tri.a],
                points[tri.b],
                points[tri.c]
            };
            cv::fillConvexPoly(colored_image, pts, 3, fill, cv::LINE_AA);
        }

        return colored_image;
    }

    std::vector<cv::Vec3b> MeanFill::EncodeColor(const cv::Mat& orig_img, const std::vector<Triangle>& triangles, const std::vector<cv::Point>& points) const {

        const int width = orig_img.cols;
        const int height = orig_img.rows;
        std::vector<cv::Vec3b> encoded_colors;
        encoded_colors.reserve(triangles.size());

        cv::Mat mask(height, width, CV_8UC1, cv::Scalar(0));

        for (const auto& tri : triangles) {
            cv::Point pts[3] = {
                points[tri.a],
                points[tri.b],
                points[tri.c]
            };
            cv::Rect roi = cv::boundingRect(std::vector<cv::Point>(pts, pts + 3))
                & cv::Rect(0, 0, width, height);

            auto mroi = mask(roi);
            mroi.setTo(0);
            cv::Point triROI[3] = { pts[0] - roi.tl(), pts[1] - roi.tl(), pts[2] - roi.tl() };
            cv::fillConvexPoly(mroi, triROI, 3, cv::Scalar(255), cv::LINE_AA);

            cv::Vec3d acc(0, 0, 0);
            int cnt = 0;
            for (int y = roi.y; y < roi.y + roi.height; ++y) {
                const uchar* mrow = mask.ptr<uchar>(y) + roi.x;
                for (int x = roi.x; x < roi.x + roi.width; ++x, ++mrow) {
                    if (!*mrow) continue;
                    cv::Vec3b px = orig_img.at<cv::Vec3b>(y, x);
                    acc += px;
                    ++cnt;
                }
            }

            cv::Vec3b color(255, 255, 255);
            if (cnt > 0) {
                acc /= cnt;
                color = cv::Vec3b(
                    static_cast<uchar>(acc[0] + 0.5),
                    static_cast<uchar>(acc[1] + 0.5),
                    static_cast<uchar>(acc[2] + 0.5));
            }
            encoded_colors.push_back(color);
        }

        return encoded_colors;
    }

    cv::Mat MeanFill::Draw(const cv::Mat& orig_img, const std::vector<Triangle>& triangles, const std::vector<cv::Point>& points) const {

        const int width = orig_img.cols;
        const int height = orig_img.rows;
        cv::Mat colored_image(height, width, CV_8UC3, cv::Scalar(255, 255, 255));

        std::vector<cv::Vec3b> colors = EncodeColor(orig_img, triangles, points);

        for (size_t i = 0; i < triangles.size(); ++i) {
            const auto& tri = triangles[i];
            const cv::Vec3b& fill_color = colors[i];
            const cv::Scalar fill(fill_color[0], fill_color[1], fill_color[2]);

            cv::Point pts[3] = {
                points[tri.a],
                points[tri.b],
                points[tri.c]
            };
            cv::fillConvexPoly(colored_image, pts, 3, fill, cv::LINE_AA);
        }

        return colored_image;
    }

    cv::Mat MeanFill::Draw(int height, int width, const std::vector<cv::Vec3b>& colors, const std::vector<Triangle>& triangles, const std::vector<cv::Point>& points) const {

        cv::Mat colored_image(height, width, CV_8UC3, cv::Scalar(255, 255, 255));

        for (size_t i = 0; i < triangles.size(); ++i) {
            const auto& tri = triangles[i];
            const cv::Vec3b& fill_color = colors[i];
            const cv::Scalar fill(fill_color[0], fill_color[1], fill_color[2]);

            cv::Point pts[3] = {
                points[tri.a],
                points[tri.b],
                points[tri.c]
            };
            cv::fillConvexPoly(colored_image, pts, 3, fill, cv::LINE_AA);
        }

        return colored_image;
    }

    std::vector<cv::Vec3b> QuantizedMeanFill::EncodeColor(const cv::Mat& orig_img, const std::vector<Triangle>& triangles, const std::vector<cv::Point>& points) const {
        // 先照 Mean 算平均，再量化每個 channel
        color::MeanFill meanfill;
        std::vector<cv::Vec3b> encoded_colors = meanfill.EncodeColor(orig_img, triangles, points);
        // 量化一次：每通道 >>3 <<3
        for (int i = 0; i < encoded_colors.size(); ++i) {
            encoded_colors[i][0] = (encoded_colors[i][0] >> 3) << 3;
            encoded_colors[i][1] = (encoded_colors[i][1] >> 3) << 3;
            encoded_colors[i][2] = (encoded_colors[i][2] >> 3) << 3;
        }
        return encoded_colors;
    }

    cv::Mat QuantizedMeanFill::Draw(const cv::Mat& orig_img, const std::vector<Triangle>& triangles, const std::vector<cv::Point>& points) const {

        // 先照 Mean 算平均，再量化每個 channel
        color::MeanFill meanfill;
        cv::Mat meanImg = meanfill.Draw(orig_img, triangles, points);
        // 量化一次：每通道 >>3 <<3
        for (int y = 0; y < meanImg.rows; ++y) {
            cv::Vec3b* row = meanImg.ptr<cv::Vec3b>(y);
            for (int x = 0; x < meanImg.cols; ++x) {
                row[x][0] = (row[x][0] >> 3) << 3;
                row[x][1] = (row[x][1] >> 3) << 3;
                row[x][2] = (row[x][2] >> 3) << 3;
            }
        }
        return meanImg;
    }

    cv::Mat QuantizedMeanFill::Draw(int height, int width, const std::vector<cv::Vec3b>& colors, const std::vector<Triangle>& triangles, const std::vector<cv::Point>& points) const {

        cv::Mat colored_image(height, width, CV_8UC3, cv::Scalar(255, 255, 255));

        for (size_t i = 0; i < triangles.size(); ++i) {
            const auto& tri = triangles[i];
            const cv::Vec3b& fill_color = colors[i];
            const cv::Scalar fill(fill_color[0], fill_color[1], fill_color[2]);

            cv::Point pts[3] = {
                points[tri.a],
                points[tri.b],
                points[tri.c]
            };
            cv::fillConvexPoly(colored_image, pts, 3, fill, cv::LINE_AA);
        }

        return colored_image;
    }

}

