#include <iostream>
#include <fstream>
#include <cmath>
#include <filesystem>

#include <opencv2/opencv.hpp>

#include "realga.h"
#include "triangulation.h"
#include "fitnessfunction.h"

namespace fs = std::filesystem;

std::vector<cv::Point> Chromosome2Points(const RealChromosome &g) {
    std::vector<cv::Point> points(g.gene.size() / 2);

    for (int i = 0; i < g.gene.size(); i += 2) {
        points[i / 2].x = floor(g.gene[i]);
        points[i / 2].y = floor(g.gene[i + 1]);
    }

    return points;
}

class PixelFitness : public FitnessFunction {
public:
    PixelFitness(const cv::Mat& original_image) : original_image_(original_image) {
        width_ = original_image_.cols;
        height_ = original_image_.rows;
        triangulation_image_builder_.set_width(width_);
        triangulation_image_builder_.set_height(height_);
    }

    float eval(const RealChromosome &g) {
        triangulation_image_builder_.RunDelaunay(Chromosome2Points(g));
        triangulation_image_builder_.DrawColoredImage(original_image_, 3);
        return triangulation_image_builder_.ComputeFitness(original_image_);
    }

private:
    int width_;
    int height_;

    cv::Mat original_image_;
    triangulation::TriangulationImageBuilder triangulation_image_builder_;
};

class ResultWriter {
public:
    void output(std::ostream& os, const std::string& label, const std::string& value) {
        os << std::left << std::setw(40) << label << value << '\n';
    }

    void output(std::ostream& os, const std::string& label, int value) {
        os << std::left << std::setw(40) << label << value << '\n';
    }

    void output(std::ostream& os, const std::string& label, unsigned long long value) {
        os << std::left << std::setw(40) << label << value << '\n';
    }

    void output(std::ostream& os, const std::string& label, float value) {
        os << std::left << std::setw(40) << label << value << '\n';
    }

    void print_settings(std::ostream& os, const RealGAOptions& options, int gen, const std::string& image_path) {
        os << "--------------- Setting ---------------\n";
        output(os, "Population Size(nInitial): ", options.populationSize);
        output(os, "Chromosome Size(ell): ", options.chromosomeSize);
        output(os, "Number of Nodes: ", options.chromosomeSize / 2);
        output(os, "Number of Generations: ", gen);
        output(os, "Image Path: ", "'" + image_path + "'");
        output(os, "Seed: ", options.seed);

        os << "Lower Bounds: [";
        for (float lb : options.lowerBounds)
            os << " " << lb;
        os << " ]\n";
        os << "Upper Bounds: [";
        for (float ub : options.upperBounds)
            os << " " << ub;
        os << " ]\n";

        output(os, "Selection Type: ",
            (options.selectionType == ROULETTE_WHEEL_SELECTION) ? "Roulette Wheel Selection" : "Tournament Selection");
        output(os, "Selection Tournament Size: ", options.selectionTournamentSize);
        output(os, "Selection Tournament Probability: ", options.selectionTournamentProbability);

        output(os, "Crossover Type: ",
            (options.crossoverType == UNIFORM_CROSSOVER) ? "Uniform Crossover" : "Single-Point Crossover");

        output(os, "Mutation Type: ",
            (options.mutationType == UNIFORM_MUTATION) ? "Uniform Mutation" : "Gaussian Mutation");
        output(os, "Mutation Rate: ", options.mutationRate);
        if (options.mutationType == UNIFORM_MUTATION) {
            output(os, "Mutation Uniform Perce: ", options.mutationUniformPerc);
        }
        else {
            output(os, "Mutation Gaussian Perc Delta: ", options.mutationGaussianPercDelta);
            output(os, "Mutation Gaussian Perc Min: ", options.mutationGaussianPercMin);
        }
        os << "---------------------------------------\n";
    }
};


int main(int argc, char *argv[]) {
    if (argc != 5) {
        std::cout << "RealGA_test numNodes nInitial gen imagePath" << std::endl;
        return -1;
    }

    const int num_nodes = std::atoi(argv[1]);
    const int nInitial = std::atoi(argv[2]);
    const int gen = std::atoi(argv[3]);
    const std::string image_path = argv[4];

    cv::Mat original_image = cv::imread(image_path, cv::IMREAD_COLOR);
    if (original_image.empty()) {
        std::cerr << "Failed to open input file: " << image_path << std::endl;
        return -1;
    }
    
    const int ell = 2 * num_nodes;
    const int width = original_image.cols;
    const int height = original_image.rows;

    std::vector<float> LB(ell);
    std::vector<float> UB(ell);
    std::fill(LB.begin(), LB.end(), 0.0);
    for (int i = 0; i < ell; i += 2) {
        UB[i] = static_cast<float>(width);
        UB[i + 1] = static_cast<float>(height);
    }
    
    PixelFitness *myFitnessFunction = new PixelFitness(original_image);
    RealGAOptions options;
    options.setChromosomeSize(ell);
    options.setPopulationSize(nInitial);
    options.setBounds(LB, UB);
    options.setVerbose("soft");

    // Save results
    triangulation::TriangulationImageBuilder triangulation_image_builder(width, height);
    std::string filename = std::string();
    for (int i = 1; i <= 3; ++i) {
        filename.append(static_cast<std::string>(argv[i]) + "_");
    }
    filename[filename.size() - 1] = '/';
    fs::create_directories(filename);
    fs::create_directories(filename + "line_images/");
    fs::create_directories(filename + "colored_images/");

    ResultWriter result_writer;
    result_writer.print_settings(std::cout, options, gen, image_path);
    std::ofstream fout(filename + "setting.txt");
    if (fout.is_open()) {
        result_writer.print_settings(fout, options, gen, image_path);
        fout.close();
    }
    else {
        std::cerr << "Failed to open output file!" << std::endl;
    }

    // Init Genetic Algorithm with options, fitness function and keepState=false
    RealGA ga;
    ga.init(options, myFitnessFunction, false);

    // Init population with uniform random between LB and UB
    ga.popInitRandUniform();

    // Evolve the population for %gen% times
    for (int i = 0; i < gen; i++) {
        std::cout << "Generation " << i + 1 << ": " << std::endl;

        ga.evolve();

        RealChromosome best = ga.getBestChromosome();
        triangulation_image_builder.RunDelaunay(Chromosome2Points(best));
        triangulation_image_builder.DrawLineImage();
        triangulation_image_builder.WriteLineImage(filename + "line_images/" + to_string(i+1) + ".jpg");
        triangulation_image_builder.DrawColoredImage(original_image, 3);
        triangulation_image_builder.WriteColoredImage(filename + "colored_images/" + to_string(i+1) + ".jpg");

        std::cout << "Best Fitness value = " << best.fitness << std::endl;
        std::cout << "---------------------------------------" << std::endl;
    }
    // get the best score function (the minimum)
    RealChromosome best = ga.getBestChromosome();

    // Print results
    cout << "Best solution: " << best.toString() << endl;
    cout << "Best Fitness value = " << best.fitness << endl;

    delete myFitnessFunction;
    return 0;
}