#ifndef COLOR_H_
#define COLOR_H_

#include <opencv2/opencv.hpp>

#include "geometry.h"

#include <vector>
#include <memory>


enum class ColorFillMode {
    kMean = 1,              // キА计
    kQuantizedMean,         // キА计秖て
    kMajority,              // 渤计
    kQuantizedMajority,     // 渤计秖て
    kBarycentric            // 絬┦础
};
std::string ToString(ColorFillMode mode);

namespace color
{

    class ColorFill {
    public:
        virtual cv::Mat Draw(const cv::Mat& orig_img, const std::vector<Triangle>& triangles, const std::vector<cv::Point>& points) const = 0;
        virtual cv::Mat Draw(int height, int width, const std::vector<cv::Vec3b>& colors, const std::vector<Triangle>& triangles, const std::vector<cv::Point>& points) const = 0;
        virtual std::vector<cv::Vec3b> EncodeColor(const cv::Mat& orig_img, const std::vector<Triangle>& triangles, const std::vector<cv::Point>& points) const = 0;
        virtual std::unique_ptr<ColorFill> clone() const = 0;
        virtual ~ColorFill() = default;
    };

    // 絬┦础
    class BarycentricFill : public ColorFill {
    public:
        BarycentricFill() {}
        std::unique_ptr<ColorFill> clone() const;
        cv::Mat Draw(const cv::Mat& orig_img, const std::vector<Triangle>& triangles, const std::vector<cv::Point>& points) const;
        cv::Mat Draw(int height, int width, const std::vector<cv::Vec3b>& colors, const std::vector<Triangle>& triangles, const std::vector<cv::Point>& points) const;
        std::vector<cv::Vec3b> EncodeColor(const cv::Mat& orig_img, const std::vector<Triangle>& triangles, const std::vector<cv::Point>& points) const;
    };

    // 渤计
    class MajorityFill : public ColorFill {
    public:
        MajorityFill() {}
        std::unique_ptr<ColorFill> clone() const;
        cv::Mat Draw(const cv::Mat& orig_img, const std::vector<Triangle>& triangles, const std::vector<cv::Point>& points) const;
        cv::Mat Draw(int height, int width, const std::vector<cv::Vec3b>& colors, const std::vector<Triangle>& triangles, const std::vector<cv::Point>& points) const;
        std::vector<cv::Vec3b> EncodeColor(const cv::Mat& orig_img, const std::vector<Triangle>& triangles, const std::vector<cv::Point>& points) const;
    };

    // 渤计秖て
    class QuantizedMajorityFill : public ColorFill {
    public:
        QuantizedMajorityFill() {}
        std::unique_ptr<ColorFill> clone() const;
        cv::Mat Draw(const cv::Mat& orig_img, const std::vector<Triangle>& triangles, const std::vector<cv::Point>& points) const;
        cv::Mat Draw(int height, int width, const std::vector<cv::Vec3b>& colors, const std::vector<Triangle>& triangles, const std::vector<cv::Point>& points) const;
        std::vector<cv::Vec3b> EncodeColor(const cv::Mat& orig_img, const std::vector<Triangle>& triangles, const std::vector<cv::Point>& points) const;
    };

    // キА计
    class MeanFill : public ColorFill {
    public:
        MeanFill() {}
        std::unique_ptr<ColorFill> clone() const;
        cv::Mat Draw(const cv::Mat& orig_img, const std::vector<Triangle>& triangles, const std::vector<cv::Point>& points) const;
        cv::Mat Draw(int height, int width, const std::vector<cv::Vec3b>& colors, const std::vector<Triangle>& triangles, const std::vector<cv::Point>& points) const;
        std::vector<cv::Vec3b> EncodeColor(const cv::Mat& orig_img, const std::vector<Triangle>& triangles, const std::vector<cv::Point>& points) const;
    };

    // キА计秖て
    class QuantizedMeanFill : public ColorFill {
    public:
        QuantizedMeanFill() {}
        std::unique_ptr<ColorFill> clone() const;
        cv::Mat Draw(const cv::Mat& orig_img, const std::vector<Triangle>& triangles, const std::vector<cv::Point>& points) const;
        cv::Mat Draw(int height, int width, const std::vector<cv::Vec3b>& colors, const std::vector<Triangle>& triangles, const std::vector<cv::Point>& points) const;
        std::vector<cv::Vec3b> EncodeColor(const cv::Mat& orig_img, const std::vector<Triangle>& triangles, const std::vector<cv::Point>& points) const;
    };

}

#endif