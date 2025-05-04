#ifndef IO_H_
#define IO_H_

#include "color.h"
#include "options.h"
#include "fitness.h"
#include "triangulation.h"

#include <iostream>
#include <string>
#include <memory>


class FitnessFunction;

class SettingBuilder {
public:
    struct ParsedSettings {
        // for filename
        std::string exp_name;

        // for 2D-poly
        std::string image_path;
        triangulation::TriangulationImageBuilder builder;

        // for GA
        ColorFillMode color_mode;
        FitnessMode fitness_mode;
        RealGAOptions options;
        std::unique_ptr<FitnessFunction> fitness_function;
    };

    // 處理輸入
    static ParsedSettings input(int argc, char* argv[]);

    // 輸出格式
    static void output(std::ostream& os, const std::string& label, const std::string& value);
    static void output(std::ostream& os, const std::string& label, int value);
    static void output(std::ostream& os, const std::string& label, unsigned long long value);
    static void output(std::ostream& os, const std::string& label, float value);

    // 輸出設定
    static void print_settings(std::ostream& os, const ParsedSettings& parsed_settings);
};

#endif