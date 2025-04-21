#include <iostream>
#include <cmath>

#include <opencv2/opencv.hpp>

#include "realga.h"
#include "triangulation.h"
#include "fitnessfunction.h"

using namespace std;

/*
std::vector<cv::Point> Chromosome2Points(const RealChromosome& g) {


    return points;
}
*/

/* Fitness function: x1^2 + x2^2 */
class QuadraticFitness : public FitnessFunction
{
public:
    QuadraticFitness() {}

    float eval(const RealChromosome &g)
    {
        return g.gene[0] * g.gene[0] + g.gene[1] * g.gene[1];
    }
};

class PixelFitness : public FitnessFunction
{
public:
    PixelFitness(cv::Mat& original_image) {
        set_original_image(original_image);
    }

    void set_original_image(cv::Mat& original_image) {
        original_image_ = original_image;
    }

    float eval(const RealChromosome& g)
    {      
        std::vector<cv::Point> points(g.gene.size() / 2);

        for (int i = 0; i < g.gene.size(); i += 2) {
            points[i / 2].x = floor(g.gene[i]);
            points[i / 2].y = floor(g.gene[i + 1]);
        }

        triangulation::TriangulationImageBuilder triangulation_builder(1200, 1200, points);
        triangulation_builder.DrawColoredImage(original_image_, 3);

        return triangulation_builder.ComputeFitness(original_image_);
    }

private:
    cv::Mat original_image_;
};



int main(int argc, char **argv)
{
    const std::string image_path = "../../dataset/01.jpg";

    const int point_num = 80;
    vector<float> LB(2 * point_num);
    vector<float> UB(2 * point_num);

    std::fill(LB.begin(), LB.end(), 0.0);
    std::fill(UB.begin(), UB.end(), 1200.0);

    cv::Mat orig_img = cv::imread(image_path, cv::IMREAD_COLOR);
    PixelFitness *myFitnessFunction = new PixelFitness(orig_img);
    RealGAOptions options;
    options.setChromosomeSize(2 * point_num);
    options.setPopulationSize(50);
    options.setBounds(LB, UB);
    options.setVerbose("soft");

    // Init Genetic Algorithm with options, fitness function and keepState=false
    RealGA ga;
    ga.init(options, myFitnessFunction, false);

    // Init population with uniform random between LB and UB
    ga.popInitRandUniform();

    // Evolve the population for 100 times
    for (int i = 0; i < 100; i++)
    {
        cout << "Generation " << i + 1 << ": " << endl;
        ga.evolve();
        RealChromosome best = ga.getBestChromosome();
        cout << "Best Fitness value = " << best.fitness << endl;
        cout << "--------------------" << endl;
    }
    // get the best score function (the minimum)
    RealChromosome best = ga.getBestChromosome();

    // Print results
    cout << "Best solution: " << best.toString() << endl;
    cout << "Best Fitness value = " << best.fitness << endl;

    std::vector<cv::Point> points(best.gene.size() / 2);

    for (int i = 0; i < best.gene.size(); i += 2) {
        points[i / 2].x = floor(best.gene[i]);
        points[i / 2].y = floor(best.gene[i + 1]);
    }

    triangulation::TriangulationImageBuilder triangulation_builder(1200, 1200, points);
    triangulation_builder.DrawColoredImage(orig_img, 3);
    triangulation_builder.WriteColoredImage("ColoredImage01.jpg");
    cv::imshow("ColoredImage01", triangulation_builder.colored_image());
    cv::waitKey(0);

    delete myFitnessFunction;
    return 0;
}