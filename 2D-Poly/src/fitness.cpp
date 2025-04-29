#include "fitness.h"

#include <vector>
#include <opencv2/opencv.hpp>

#include "chromosome.h"


std::vector<cv::Point> PixelFitness::Chromosome2Points(const RealChromosome& g) {
    std::vector<cv::Point> points(g.gene.size() / 2);

    for (int i = 0; i < g.gene.size(); i += 2) {
        points[i / 2].x = floor(g.gene[i]);
        points[i / 2].y = floor(g.gene[i + 1]);
    }

    return points;
}

PixelFitness::PixelFitness(const cv::Mat& original_image) : original_image_(original_image) {
    width_ = original_image_.cols;
    height_ = original_image_.rows;
    triangulation_image_builder_.set_width(width_);
    triangulation_image_builder_.set_height(height_);
}

float PixelFitness::eval(const RealChromosome& g) {
    triangulation_image_builder_.RunDelaunay(Chromosome2Points(g));
    triangulation_image_builder_.DrawColoredImage(original_image_, 3);
    return triangulation_image_builder_.ComputeFitness(original_image_);
}

