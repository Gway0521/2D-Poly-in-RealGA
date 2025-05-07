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

    // Other variables
    const int kNumImagesToSave = 100;
    int save_interval = std::max(1, static_cast<int>(options.gen) / kNumImagesToSave);
    std::unique_ptr<fitness::MSE> MSE_function = std::make_unique<fitness::MSE>(builder.original_image(), builder.clone_color_fill());
    std::unique_ptr<fitness::PSNR> PSNR_function = std::make_unique<fitness::PSNR>(builder.original_image(), builder.clone_color_fill());
    std::unique_ptr<fitness::SSIM> SSIM_function = std::make_unique<fitness::SSIM>(builder.original_image(), builder.clone_color_fill());

    // Make file for saving results
    std::string filename = exp_name + '/';
    fs::create_directories(filename);
    fs::create_directories(filename + "line_images/");
    fs::create_directories(filename + "colored_images/");
    fs::create_directories(filename + "compressed_images/");

    // Print and write settings
    SettingBuilder::print_settings(std::cout, parsed_settings);
    std::ofstream fout(filename + "setting.txt");
    if (fout.is_open())
        SettingBuilder::print_settings(fout, parsed_settings);
    else
        std::cerr << "Failed to open output file!" << std::endl;

    // Init Genetic Algorithm with options, fitness function and keepState=false
    RealGA ga;
    ga.init(options, fitness_function, false);

    // Init population with uniform random between LB and UB
    ga.popInitRandUniform();

    // Evolve the population for %gen% times
    for (int i = 0; i < options.gen; i++) {
        std::cout << "Generation " << i + 1 << ": " << std::endl;
        fout << "Generation " << i + 1 << ": " << std::endl;

        ga.evolve();

        RealChromosome best = ga.getBestChromosome();
        builder.RunDelaunay(fitness::Chromosome2Points(best));
        builder.DrawLineImage();
        builder.DrawColoredImage();
        if (i % save_interval == 0) {
            builder.WriteLineImage(filename + "line_images/" + to_string(i + 1) + "_" + to_string(best.fitness) + ".jpg");
            builder.WriteColoredImage(filename + "colored_images/" + to_string(i + 1) + "_" + to_string(best.fitness) + ".jpg");
            int file_size = builder.Encode(filename + "compressed_images/" + to_string(i + 1) + "_" + to_string(best.fitness) + ".gaimg");
            std::cout << "File size: " << file_size << " bytes" << std::endl;
        }

        SettingBuilder::print_results(std::cout, best, MSE_function.get(), PSNR_function.get(), SSIM_function.get());
        SettingBuilder::print_results(fout, best, MSE_function.get(), PSNR_function.get(), SSIM_function.get());
    }
    // get the best score function (the minimum)
    RealChromosome best = ga.getBestChromosome();

    // encode, decode, and print results
    builder.RunDelaunay(fitness::Chromosome2Points(best));
    builder.DrawLineImage();
    builder.DrawColoredImage();
    builder.Encode(filename + "compressed_images/" "best_" + to_string(best.fitness) + ".gaimg");
    int file_size = builder.Decode(filename + "compressed_images/" "best_" + to_string(best.fitness) + ".gaimg");

    cout << "Best solution: " << best.toString() << endl;
    cout << "Best Fitness value = " << best.fitness << endl;
    std::cout << "File size: " << file_size << " bytes" << std::endl;
    cv::imshow("Decoded Image", builder.colored_image());
    cv::waitKey(0);

    fout.close();

    return 0;
}