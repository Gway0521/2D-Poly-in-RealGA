#ifndef ___TRIANGULATION_H
#define ___TRIANGULATION_H

#include <opencv2/opencv.hpp>

#include <string>
#include <vector>

struct Triangle
{
    int a, b, c;
};

struct Edge
{
    int a, b;
};

bool operator==(const Edge &e1, const Edge &e2);

double orientation(const cv::Point &a, const cv::Point &b, const cv::Point &c);

class DTImage
{
public:
    DTImage(int w, int h);
    DTImage(int w, int h, const std::vector<cv::Point> &points);

    void delaunayTriangulation(const std::vector<cv::Point> &points);

    void drawLine();
    void drawColor(const cv::Mat &origImg);

    int getNumTriangles() const;
    void printTriangles() const;
    void writeLineImg(const std::string filename) const;
    void writeColoredImg(const std::string filename) const;

private:
    int width;
    int height;

    std::vector<cv::Point> points;
    std::vector<Triangle> triangles;

    cv::Mat lineImg;
    cv::Mat coloredImg;
};

#endif
