#include "fitness.h"

#include <vector>
#include <opencv2/opencv.hpp>
#include <iostream>

#include "chromosome.h"
#include "triangulation.h"


namespace fitness
{
    std::vector<cv::Point> Chromosome2Points(const RealChromosome& g) {
        std::vector<cv::Point> points(g.gene.size() / 2);

        for (int i = 0; i < g.gene.size(); i += 2) {
            points[i / 2].x = floor(g.gene[i]);
            points[i / 2].y = floor(g.gene[i + 1]);
        }

        return points;
    }

    MSE::MSE(const cv::Mat& original_image, int mode) : builder_(triangulation::TriangulationImageBuilder(original_image, mode)) {}
    PSNR::PSNR(const cv::Mat& original_image, int mode) : mse_fitness_function(MSE(original_image, mode)) {}
    SSIM::SSIM(const cv::Mat& original_image, int mode) : builder_(triangulation::TriangulationImageBuilder(original_image, mode)) {}

    // MSE
    float MSE::eval(const RealChromosome& g) {

        // Draw Delaunay Triangulation
        std::vector<cv::Point> points = Chromosome2Points(g);

        builder_.RunDelaunay(points);
        builder_.DrawColoredImage();

        // Difference
        cv::Mat diff;
        cv::absdiff(builder_.original_image(), builder_.colored_image(), diff);
        diff.convertTo(diff, CV_32F);

        // Square
        diff = diff.mul(diff);

        // Accumulate
        cv::Scalar s = cv::sum(diff);
        double mse = 0.0;
        for (int i = 0; i < diff.channels(); ++i)
            mse += s[i];

        // Average
        mse /= (builder_.original_image().total() * builder_.original_image().channels());

        return mse;
    }

    // PSNR
    float PSNR::CalPSNR(const RealChromosome& g) {

        // MSE
        float mse = mse_fitness_function.eval(g);
        if (mse <= 0.0f) {
            return std::numeric_limits<float>::infinity();
        }

        // PSNR
        constexpr float kMaxI = 255.0f;
        float psnr = 10.0f * std::log10((kMaxI * kMaxI) / mse);
        return psnr;
    }

    float PSNR::eval(const RealChromosome& g) {
        float psnr = CalPSNR(g);
        return 1 / psnr;
    }

    // SSIM ( window size = 11กั11 )
    float SSIM::CalSSIM(const RealChromosome& g) {

        // Draw Delaunay Triangulation
        std::vector<cv::Point> points = Chromosome2Points(g);
        builder_.RunDelaunay(points);
        builder_.DrawColoredImage();

        // Convert both images to 32F grayscale
        cv::Mat img1, img2;
        cv::cvtColor(builder_.original_image(), img1, cv::COLOR_BGR2GRAY);
        cv::cvtColor(builder_.colored_image(), img2, cv::COLOR_BGR2GRAY);
        img1.convertTo(img1, CV_32F);
        img2.convertTo(img2, CV_32F);

        // Constants
        const float kC1 = (0.01f * 255.0f) * (0.01f * 255.0f);
        const float kC2 = (0.03f * 255.0f) * (0.03f * 255.0f);

        // Means
        cv::Mat mu1, mu2;
        cv::GaussianBlur(img1, mu1, cv::Size(11, 11), 1.5);
        cv::GaussianBlur(img2, mu2, cv::Size(11, 11), 1.5);

        // Squares of means
        cv::Mat mu1_sq = mu1.mul(mu1);
        cv::Mat mu2_sq = mu2.mul(mu2);
        cv::Mat mu1_mu2 = mu1.mul(mu2);

        // Variances and covariance
        cv::Mat sigma1_sq, sigma2_sq, sigma12;
        cv::GaussianBlur(img1.mul(img1), sigma1_sq, cv::Size(11, 11), 1.5);
        sigma1_sq -= mu1_sq;
        cv::GaussianBlur(img2.mul(img2), sigma2_sq, cv::Size(11, 11), 1.5);
        sigma2_sq -= mu2_sq;
        cv::GaussianBlur(img1.mul(img2), sigma12, cv::Size(11, 11), 1.5);
        sigma12 -= mu1_mu2;

        // SSIM map
        cv::Mat t1 = 2 * mu1_mu2 + kC1;
        cv::Mat t2 = 2 * sigma12 + kC2;
        cv::Mat t3 = mu1_sq + mu2_sq + kC1;
        cv::Mat t4 = sigma1_sq + sigma2_sq + kC2;

        cv::Mat ssim_map;
        cv::divide(t1.mul(t2), t3.mul(t4), ssim_map);

        // Average
        cv::Scalar mssim = cv::mean(ssim_map);
        return static_cast<float>(mssim[0]);
    }

    float SSIM::eval(const RealChromosome& g) {
        float ssim = CalSSIM(g);
        return 1 - ssim;
    }
}

