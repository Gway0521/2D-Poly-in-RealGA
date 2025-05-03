#ifndef IO_H_
#define IO_H_

#include <iostream>
#include <string>

#include "options.h"
#include "fitnessfunction.h"
#include "triangulation.h"


class SettingBuilder {
public:
    struct Ret {
        // for filename
        string exp_name;

        // for 2D-poly
        string image_path;
        triangulation::TriangulationImageBuilder builder;

        // for GA
        RealGAOptions options;
        FitnessFunction* fitness_function;

        ~Ret() {
            delete fitness_function;
        }
    };

    static Ret input(int argc, char* argv[]);

    static void output(std::ostream& os, const std::string& label, const std::string& value);
    static void output(std::ostream& os, const std::string& label, int value);
    static void output(std::ostream& os, const std::string& label, unsigned long long value);
    static void output(std::ostream& os, const std::string& label, float value);

    static void print_settings(std::ostream& os, const RealGAOptions& options, const std::string& image_path);


    static int mode_num;
    static string fitness_function_name;
};

#endif