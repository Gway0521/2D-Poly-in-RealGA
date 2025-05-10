#include "io.h"

#include <iostream>
#include <memory>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <string>

#include <opencv2/opencv.hpp>

#include "options.h"
#include "fitness.h"
#include "color.h"
#include "triangulation.h"


SettingBuilder::ParsedSettings SettingBuilder::input(int argc, char* argv[]) {
    if (argc == 1 || argc % 2 == 0 || std::string(argv[1]) == "-help") {
        std::cerr << "Usage: RealGA_test -imagePath <string> -expName <string>\n"
            << "[-numNodes <int>] [-nInitial <int>] [-gen <int>]\n"
            << "[-selectionType <string>] [-tournamentSize <int>] [-tournamentProb <float>]\n"
            << "[-crossoverType <string>] [-BLXAlpha <float>]\n"
            << "[-mutationType <string>] [-mutationRate <float>] [-mutationUniformPerc <float>] [-mutateDuplicatedFitness <bool>]\n"
            << "[-colorMode <int>] [-fitnessFunction <string>]\n"
            << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // store input
    static std::unordered_map<std::string, std::string> args;

    // check input key
    const std::unordered_set<std::string> validKeys = {
        "-imagePath", "-expName", "-numNodes", "-nInitial", "-gen",
        "-selectionType", "-tournamentSize", "-tournamentProb",
        "-crossoverType", "-BLXAlpha",
        "-mutationType", "-mutationRate", "-mutationUniformPerc", "-mutateDuplicatedFitness",
        "-colorMode", "-fitnessFunction"
    };
    for (int i = 1; i < argc; i += 2) {
        if (validKeys.count(std::string(argv[i])) > 0)
            args[argv[i]] = argv[i + 1];
        else {
            std::cerr << argv[i] << " is an invalid input key" << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }

    ParsedSettings parsed_settings;

    // Required
    bool flag = false;
    if (args.count("-imagePath") == 0) {
        std::cout << "Require -imagePath <image_path>" << std::endl;
        flag = true;
    }
    else
        parsed_settings.image_path = args["-imagePath"];
    if (args.count("-expName") == 0) {
        std::cout << "Require -expName <expName>" << std::endl;
        flag = true;
    }   
    else
        parsed_settings.exp_name = args["-expName"];
    if (flag) std::exit(EXIT_FAILURE);

    // basic options
    if (args.count("-numNodes") != 0) parsed_settings.options.setChromosomeSize(std::stoi(args["-numNodes"]));
    else parsed_settings.options.setChromosomeSize(80);
    if (args.count("-nInitial") != 0) parsed_settings.options.setPopulationSize(std::stoi(args["-nInitial"]));
    else parsed_settings.options.setPopulationSize(100);
    if (args.count("-gen") != 0) parsed_settings.options.setGeneration(std::stoi(args["-gen"]));
    else parsed_settings.options.setGeneration(100);

    // image
    cv::Mat original_image = cv::imread(args["-imagePath"], cv::IMREAD_COLOR);
    if (original_image.empty()) {
        std::cerr << "Failed to open input file: " << args["-imagePath"] << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // selection
    if (args.count("-selectionType") != 0) parsed_settings.options.setSelectionType(args["-selectionType"]);
    else parsed_settings.options.setSelectionType("tournament");
    if (args.count("-tournamentSize") != 0) parsed_settings.options.setSelectionTournamentSize(std::stoi(args["-tournamentSize"]));
    else parsed_settings.options.setSelectionTournamentSize(2);
    if (args.count("-tournamentProb") != 0) parsed_settings.options.setSelectionTournamentProbability(std::stof(args["-tournamentProb"]));
    else parsed_settings.options.setSelectionTournamentProbability(1);

    // crossover
    if (args.count("-crossoverType") != 0) parsed_settings.options.setCrossoverType(args["-crossoverType"]);
    else parsed_settings.options.setCrossoverType("BLX1p");
    if (args.count("-BLXAlpha") != 0) parsed_settings.options.setBLX_alpha(std::stof(args["-BLXAlpha"]));
    else parsed_settings.options.setBLX_alpha(0.02);

    // mutation
    if (args.count("-mutationType") != 0) parsed_settings.options.setMutationType(args["-mutationType"]);
    else parsed_settings.options.setMutationType("uniform");
    if (args.count("-mutationRate") != 0) parsed_settings.options.setMutationRate(std::stof(args["-mutationRate"]));
    else parsed_settings.options.setMutationRate(0.0125);
    if (args.count("-mutationUniformPerc") != 0) parsed_settings.options.setUniformMutationRate(std::stof(args["-mutationUniformPerc"]));
    else parsed_settings.options.setUniformMutationRate(0.25);
    if (args.count("-mutateDuplicatedFitness") != 0)
        if (args["-mutateDuplicatedFitness"] == "false" || args["-mutateDuplicatedFitness"] == "False" || args["-mutateDuplicatedFitness"] == "0")
            parsed_settings.options.setMutateDuplicatedFitness(false);
        else
            parsed_settings.options.setMutateDuplicatedFitness(true);

    // color mode (default: Quantized Mean (mode 2)), fitness function (default PSNR) and builder
    parsed_settings.color_mode = ColorFillMode::kQuantizedMean;
    std::unique_ptr<color::ColorFill> color_fill;

    if (args.count("-colorMode") != 0)
        parsed_settings.color_mode = static_cast<ColorFillMode>(std::stoi(args["-colorMode"]));
    if (parsed_settings.color_mode == ColorFillMode::kMean)
        color_fill = std::make_unique<color::MeanFill>();
    else if (parsed_settings.color_mode == ColorFillMode::kQuantizedMean)
        color_fill = std::make_unique<color::QuantizedMeanFill>();
    else if (parsed_settings.color_mode == ColorFillMode::kMajority)
        color_fill = std::make_unique<color::MajorityFill>();
    else if (parsed_settings.color_mode == ColorFillMode::kQuantizedMajority)
        color_fill = std::make_unique<color::QuantizedMajorityFill>();
    else
        color_fill = std::make_unique<color::BarycentricFill>();

    if (args.count("-fitnessFunction") != 0) {
        if (args["-fitnessFunction"] == "MSE") {
            parsed_settings.fitness_mode = FitnessMode::kMSE;
            parsed_settings.fitness_function = std::make_unique<fitness::MSE>(original_image, color_fill->clone());
        }
        else if (args["-fitnessFunction"] == "SSIM") {
            parsed_settings.fitness_mode = FitnessMode::kSSIM;
            parsed_settings.fitness_function = std::make_unique<fitness::SSIM>(original_image, color_fill->clone());
        }
        else {
            parsed_settings.fitness_mode = FitnessMode::kPSNR;
            parsed_settings.fitness_function = std::make_unique<fitness::PSNR>(original_image, color_fill->clone());
        }
    }
    else {
        parsed_settings.fitness_mode = FitnessMode::kMSE;
        parsed_settings.fitness_function = std::make_unique<fitness::MSE>(original_image, color_fill->clone());
    }
        
    parsed_settings.builder = triangulation::TriangulationImageBuilder(original_image, move(color_fill));
    parsed_settings.builder.RunEdgeDetection(0.9);

    // LB and UB
    size_t ell = parsed_settings.options.chromosomeSize;
    std::vector<float> LB(ell);
    std::vector<float> UB(ell);
    std::fill(LB.begin(), LB.end(), 0.0);
    for (int i = 0; i < ell; ++i)
        UB[i] = static_cast<float>(parsed_settings.builder.gene_to_point().size() - 1);
    parsed_settings.options.setBounds(LB, UB);

    return parsed_settings;
}

void SettingBuilder::output(std::ostream& os, const std::string& label, const std::string& value) {
    os << std::left << std::setw(40) << label << value << '\n';
}
void SettingBuilder::output(std::ostream& os, const std::string& label, int value) {
    os << std::left << std::setw(40) << label << value << '\n';
}
void SettingBuilder::output(std::ostream& os, const std::string& label, unsigned long long value) {
    os << std::left << std::setw(40) << label << value << '\n';
}
void SettingBuilder::output(std::ostream& os, const std::string& label, float value) {
    os << std::left << std::setw(40) << label << value << '\n';
}

void SettingBuilder::print_settings(std::ostream& os, const ParsedSettings& parsed_settings) {
    const RealGAOptions& options = parsed_settings.options;
    const std::string& image_path = parsed_settings.image_path;
    const ColorFillMode& color_mode = parsed_settings.color_mode;
    const FitnessMode& fitness_mode = parsed_settings.fitness_mode;

    os << "--------------- Setting ---------------\n";
    output(os, "Population Size(nInitial): ", static_cast<unsigned long long>(options.populationSize));
    output(os, "Chromosome Size(ell): ", static_cast<unsigned long long>(options.chromosomeSize));
    output(os, "Number of Nodes: ", static_cast<unsigned long long>(options.chromosomeSize));
    output(os, "Number of Generations: ", static_cast<unsigned long long>(options.gen));
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

    if (options.crossoverType == UNIFORM_CROSSOVER) {
        output(os, "Crossover Type: ", "Uniform Crossover");
    }
    else {
        output(os, "Crossover Type: ", "BLX1p Crossover");
        output(os, "BLX Alpha: ", options.BLX_alpha);
    }

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

    output(os, "Color Mode: ", ToString(color_mode));
    output(os, "Fitness Function: ", ToString(fitness_mode));
    os << "---------------------------------------\n\n\n";
}

void SettingBuilder::print_results(std::ostream& os, const RealChromosome& best, fitness::MSE* mse_f, fitness::PSNR* psnr_f, fitness::SSIM* ssim_f) {
    os << std::left << std::setw(40) << "Best Fitness value = " << best.fitness << '\n';
    os << std::left << std::setw(40) << "MSE Fitness value = " << mse_f->eval(best) << '\n';
    os << std::left << std::setw(40) << "PSNR Fitness value = " << psnr_f->eval(best) << '\n';
    os << std::left << std::setw(40) << "SSIM Fitness value = " << ssim_f->eval(best) << '\n';
    os << std::left << std::setw(40) << "PSNR value = " << psnr_f->CalPSNR(best) << '\n';
    os << std::left << std::setw(40) << "SSIM value = " << ssim_f->CalSSIM(best) << '\n';
    os << "---------------------------------------\n\n";
}
