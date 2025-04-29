#include <iostream>
#include <fstream>
#include <cmath>
#include <filesystem>

#include <opencv2/opencv.hpp>

#include "realga.h"
#include "triangulation.h"
#include "io.h"

namespace fs = std::filesystem;


int main(int argc, char *argv[]) {

    // Process inputs
    auto [exp_name, image_path, options, fitness_function] = SettingBuilder::input(argc, argv);

    // Read image
    cv::Mat original_image = cv::imread(image_path, cv::IMREAD_COLOR);
    if (original_image.empty()) {
        std::cerr << "Failed to open input file: " << image_path << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // Make file for saving results
    std::string filename = exp_name + '/';
    fs::create_directories(filename);
    fs::create_directories(filename + "line_images/");
    fs::create_directories(filename + "colored_images/");

    // Print and write settings
    SettingBuilder::print_settings(std::cout, options, image_path);
    std::ofstream fout(filename + "setting.txt");
    if (fout.is_open()) {
        SettingBuilder::print_settings(fout, options, image_path);
        fout.close();
    }
    else {
        std::cerr << "Failed to open output file!" << std::endl;
    }

    // Init Triangulation Builder for making result
    triangulation::TriangulationImageBuilder triangulation_image_builder(original_image.cols, original_image.rows);

    // Init Genetic Algorithm with options, fitness function and keepState=false
    RealGA ga;
    ga.init(options, fitness_function, false);

    // Init population with uniform random between LB and UB
    ga.popInitRandUniform();

    // Evolve the population for %gen% times
    for (int i = 0; i < options.gen; i++) {
        std::cout << "Generation " << i + 1 << ": " << std::endl;

        ga.evolve();

        RealChromosome best = ga.getBestChromosome();
        triangulation_image_builder.RunDelaunay(PixelFitness::Chromosome2Points(best));
        triangulation_image_builder.DrawLineImage();
        triangulation_image_builder.WriteLineImage(filename + "line_images/" + to_string(i+1) + "_" + to_string(best.fitness) + ".jpg");
        triangulation_image_builder.DrawColoredImage(original_image, 3);
        triangulation_image_builder.WriteColoredImage(filename + "colored_images/" + to_string(i+1) + "_" + to_string(best.fitness) + ".jpg");

        std::cout << "Best Fitness value = " << best.fitness << std::endl;
        std::cout << "---------------------------------------" << std::endl;
    }
    // get the best score function (the minimum)
    RealChromosome best = ga.getBestChromosome();

    // Print results
    cout << "Best solution: " << best.toString() << endl;
    cout << "Best Fitness value = " << best.fitness << endl;

    delete fitness_function;
    return 0;
}