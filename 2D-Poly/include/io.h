#ifndef IO_H_
#define IO_H_

#include <iostream>
#include <string>

#include "options.h"
#include "fitness.h"


class SettingBuilder {
public:
    struct Ret {
        // for filename
        string exp_name;

        // for 2D-poly
        string image_path;

        // for GA
        RealGAOptions options;
        PixelFitness* myFitnessFunction;

        ~Ret() {
            delete myFitnessFunction;
        }
    };

    static Ret input(int argc, char* argv[]);

    static void output(std::ostream& os, const std::string& label, const std::string& value);
    static void output(std::ostream& os, const std::string& label, int value);
    static void output(std::ostream& os, const std::string& label, unsigned long long value);
    static void output(std::ostream& os, const std::string& label, float value);

    static void print_settings(std::ostream& os, const RealGAOptions& options, const std::string& image_path);
};

#endif