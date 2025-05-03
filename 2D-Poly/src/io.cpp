#include "io.h"

#include <iostream>
#include <vector>
#include <unordered_map>
#include <string>

#include <opencv2/opencv.hpp>

#include "options.h"
#include "fitness.h"
#include "triangulation.h"

int SettingBuilder::mode_num = 0;
std::string SettingBuilder::fitness_function_name = "";

SettingBuilder::Ret SettingBuilder::input(int argc, char* argv[]) {
    if (argc % 2 == 0) {
        std::cerr << "Usage: RealGA_test -image_path <string> -expName <string> "
            << "[-numNodes <int>] [-nInitial <int>] [-gen <int>] "
            << "[-selectionType <string>] [-tournamentSize <int>] [-tournamentProb <float>] "
            << "[-crossoverType <string>] [-BLXAlpha <float>] "
            << "[-mutationType <string>] [-mutationRate <float>] [-mutateDuplicatedFitness <bool>] "
            << "[-colorMode <int>] [-fitnessFunction <string>]"
            << std::endl;
        std::exit(EXIT_FAILURE);
    }
    
    std::unordered_map<std::string, std::string> args;
    for (int i = 1; i < argc; i += 2)
        args[argv[i]] = argv[i + 1];


    Ret ret;

    // Required
    bool flag = false;
    if (args.count("-image_path") == 0) {
        std::cout << "Require -image_path <image_path>" << std::endl;
        flag = true;
    }
    else
        ret.image_path = args["-image_path"];
    if (args.count("-expName") == 0) {
        std::cout << "Require -expName <expName>" << std::endl;
        flag = true;
    }   
    else
        ret.exp_name = args["-expName"];
    if (flag) std::exit(EXIT_FAILURE);

    // basic options
    if (args.count("-numNodes") != 0) ret.options.setChromosomeSize(std::stoi(args["-numNodes"]) * 2);
    if (args.count("-nInitial") != 0) ret.options.setPopulationSize(std::stoi(args["-nInitial"]));
    if (args.count("-gen") != 0) ret.options.setGeneration(std::stoi(args["-gen"]));

    // image
    cv::Mat original_image = cv::imread(args["-image_path"], cv::IMREAD_COLOR);
    if (original_image.empty()) {
        std::cerr << "Failed to open input file: " << args["-image_path"] << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // LB and UB
    size_t ell = ret.options.chromosomeSize;
    std::vector<float> LB(ell);
    std::vector<float> UB(ell);
    std::fill(LB.begin(), LB.end(), 0.0);
    for (int i = 0; i < ell; i += 2) {
        UB[i] = static_cast<float>(original_image.cols);
        UB[i + 1] = static_cast<float>(original_image.rows);
    }
    ret.options.setBounds(LB, UB);

    // selection
    if (args.count("-selectionType") != 0) ret.options.setSelectionType(args["-selectionType"]);
    if (args.count("-tournamentSize") != 0) ret.options.setSelectionTournamentSize(std::stoi(args["-tournamentSize"]));
    if (args.count("-tournamentProb") != 0) ret.options.setSelectionTournamentProbability(std::stof(args["-tournamentProb"]));

    // crossover
    if (args.count("-crossoverType") != 0) ret.options.setCrossoverType(args["-crossoverType"]);
    if (args.count("-BLXAlpha") != 0) ret.options.setBLX_alpha(std::stof(args["-BLXAlpha"]));

    // mutation
    if (args.count("-mutationType") != 0) ret.options.setMutationType(args["-mutationType"]);
    if (args.count("-mutationRate") != 0) ret.options.setMutationRate(std::stof(args["-mutationRate"]));
    if (args.count("-mutateDuplicatedFitness") != 0)
        if (args["-mutateDuplicatedFitness"] == "false" || args["-mutateDuplicatedFitness"] == "False" || args["-mutateDuplicatedFitness"] == "0")
            ret.options.setMutateDuplicatedFitness(false);
        else
            ret.options.setMutateDuplicatedFitness(true);

    // color mode (default 3), fitness function (default PSNR) and builder
    int mode = 3;
    if (args.count("-colorMode") != 0)
        mode = std::stoi(args["-colorMode"]);
    mode_num = mode;

    if (args.count("-fitnessFunction") != 0) {
        if (args["-fitnessFunction"] == "MSE") {
            fitness_function_name = "MSE";
            ret.fitness_function = new fitness::MSE(original_image, mode);
        }
        else if (args["-fitnessFunction"] == "SSIM") {
            fitness_function_name = "SSIM";
            ret.fitness_function = new fitness::SSIM(original_image, mode);
        }
        else {
            fitness_function_name = "PSNR";
            ret.fitness_function = new fitness::PSNR(original_image, mode);
        }
    }
    else {
        fitness_function_name = "PSNR";
        ret.fitness_function = new fitness::PSNR(original_image, mode);
    }
        
    ret.builder = triangulation::TriangulationImageBuilder(original_image, mode);

    return ret;
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

void SettingBuilder::print_settings(std::ostream& os, const RealGAOptions& options, const std::string& image_path) {
    os << "--------------- Setting ---------------\n";
    output(os, "Population Size(nInitial): ", static_cast<unsigned long long>(options.populationSize));
    output(os, "Chromosome Size(ell): ", static_cast<unsigned long long>(options.chromosomeSize));
    output(os, "Number of Nodes: ", static_cast<unsigned long long>(options.chromosomeSize / 2));
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

    output(os, "Mode: ", mode_num);
    output(os, "Fitness Function: ", fitness_function_name);
    os << "---------------------------------------\n";
}
