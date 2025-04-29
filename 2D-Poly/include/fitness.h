#ifndef FITNESS_H_
#define FITNESS_H_

#include <vector>
#include <opencv2/opencv.hpp>

#include "fitnessfunction.h"
#include "triangulation.h"
#include "chromosome.h"


class PixelFitness : public FitnessFunction {
public:
    PixelFitness(const cv::Mat& original_image);

    static std::vector<cv::Point> Chromosome2Points(const RealChromosome& g);
    float eval(const RealChromosome& g);

private:
    int width_;
    int height_;

    cv::Mat original_image_;
    triangulation::TriangulationImageBuilder triangulation_image_builder_;
};

#endif