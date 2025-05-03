#include <iostream>
#include <fstream>
#include <cmath>
#include <filesystem>

#include <opencv2/opencv.hpp>

#include "realga.h"
#include "fitness.h"
#include "triangulation.h"
#include "io.h"

namespace fs = std::filesystem;


int main(int argc, char *argv[]) {

    // Process inputs
    auto [exp_name, image_path, builder, options, fitness_function] = SettingBuilder::input(argc, argv);
    const int how_many_images_do_i_need_to_save = 100;

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
        if (i % (options.gen / how_many_images_do_i_need_to_save) == 0) {
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

    delete fitness_function;
    return 0;
}