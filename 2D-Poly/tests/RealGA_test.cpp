#include <opencv2/opencv.hpp>

#include "realga.h"
#include "fitness.h"
#include "triangulation.h"
#include "io.h"

#include <iostream>
#include <string>
#include <fstream>
#include <filesystem>


namespace fs = std::filesystem;


int main(int argc, char *argv[]) {

    // Process inputs
    SettingBuilder::ParsedSettings parsed_settings = SettingBuilder::input(argc, argv);
    const std::string& exp_name = parsed_settings.exp_name;
    RealGAOptions& options = parsed_settings.options;
    FitnessFunction* fitness_function = parsed_settings.fitness_function.get();
    triangulation::TriangulationImageBuilder& builder = parsed_settings.builder;

    const int kNumImagesToSave = 100;
    int save_interval = std::max(1, static_cast<int>(options.gen) / kNumImagesToSave);

    // Make file for saving results
    std::string filename = exp_name + '/';
    fs::create_directories(filename);
    fs::create_directories(filename + "line_images/");
    fs::create_directories(filename + "colored_images/");

    // Print and write settings
    SettingBuilder::print_settings(std::cout, parsed_settings);
    std::ofstream fout(filename + "setting.txt");
    if (fout.is_open()) {
        SettingBuilder::print_settings(fout, parsed_settings);
        fout.close();
    }
    else {
        std::cerr << "Failed to open output file!" << std::endl;
    }

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
        builder.RunDelaunay(fitness::Chromosome2Points(best));
        builder.DrawLineImage();
        builder.DrawColoredImage();
        if (i % save_interval == 0) {
            builder.WriteLineImage(filename + "line_images/" + to_string(i + 1) + "_" + to_string(best.fitness) + ".jpg");
            builder.WriteColoredImage(filename + "colored_images/" + to_string(i + 1) + "_" + to_string(best.fitness) + ".jpg");
        }

        std::cout << "Best Fitness value = " << best.fitness << std::endl;
        std::cout << "---------------------------------------" << std::endl;
    }
    // get the best score function (the minimum)
    RealChromosome best = ga.getBestChromosome();

    // Print results
    cout << "Best solution: " << best.toString() << endl;
    cout << "Best Fitness value = " << best.fitness << endl;

    return 0;
}