#ifndef FITNESS_H_
#define FITNESS_H_

#include "fitnessfunction.h"
#include "triangulation.h"

#include <vector>
#include <memory>


namespace cv { class Mat; }
namespace color { class ColorFill; }
class  RealChromosome;

enum class FitnessMode {
    kMSE = 1,
    kPSNR,
    kSSIM,
};
std::string ToString(FitnessMode mode);

namespace fitness
{

    std::vector<cv::Point> Chromosome2Points(const RealChromosome& g, const vector<cv::Point>& gene_to_point);

    // MSE
    class MSE : public FitnessFunction {
    public:
        MSE(const cv::Mat& original_image, std::unique_ptr<color::ColorFill> color_fill);
        float eval(const RealChromosome& g);

    private:
        triangulation::TriangulationImageBuilder builder_;
    };

    // PSNR
    class PSNR : public FitnessFunction {
    public:
        PSNR(const cv::Mat& original_image, std::unique_ptr<color::ColorFill> color_fill);
        float CalPSNR(const RealChromosome& g);
        float eval(const RealChromosome& g);

    private:
        MSE mse_fitness_function_;
    };

    // SSIM
    class SSIM : public FitnessFunction {
    public:
        SSIM(const cv::Mat& original_image, std::unique_ptr<color::ColorFill> color_fill);
        float CalSSIM(const RealChromosome& g);
        float eval(const RealChromosome& g);

    private:
        triangulation::TriangulationImageBuilder builder_;
    };

}

#endif