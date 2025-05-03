#ifndef FITNESS_H_
#define FITNESS_H_

#include <vector>
#include <opencv2/opencv.hpp>

#include "fitnessfunction.h"
#include "triangulation.h"
#include "chromosome.h"
#include "color.h"


namespace fitness
{

    std::vector<cv::Point> Chromosome2Points(const RealChromosome& g);

    // MSE
    class MSE : public FitnessFunction {
    public:
        MSE(const cv::Mat& original_image, unique_ptr<color::ColorFill> color_fill);
        float eval(const RealChromosome& g);

    private:
        triangulation::TriangulationImageBuilder builder_;
    };

    // PSNR
    class PSNR : public FitnessFunction {
    public:
        PSNR(const cv::Mat& original_image, unique_ptr<color::ColorFill> color_fill);
        float CalPSNR(const RealChromosome& g);
        float eval(const RealChromosome& g);

    private:
        MSE mse_fitness_function;
    };

    // SSIM
    class SSIM : public FitnessFunction {
    public:
        SSIM(const cv::Mat& original_image, unique_ptr<color::ColorFill> color_fill);
        float CalSSIM(const RealChromosome& g);
        float eval(const RealChromosome& g);

    private:
        triangulation::TriangulationImageBuilder builder_;
    };

}

#endif