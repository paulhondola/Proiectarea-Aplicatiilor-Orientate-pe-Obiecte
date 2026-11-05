#include "Demos.hpp"
#include "Filters.hpp"
#include "ImageIO.hpp"
#include "Logger.hpp"

#include <CLI/CLI.hpp>

#include <exception>
#include <iostream>
#include <string>
#include <vector>

namespace {

void runDemos(int lab) {
    Logger::instance().setEnabled(true);
    if (lab == 0 || lab == 3) {
        runLab3();
    }
    if (lab == 0 || lab == 4) {
        runLab4();
    }
    if (lab == 0 || lab == 5) {
        runLab5();
    }
    std::cout << "\nAll checks passed.\n";
}

void runFilters(const std::string& input, const std::string& output,
                const std::vector<std::string>& filters) {
    const LabeledImage loaded = loadImage(input);
    Image current = loaded;
    for (const std::string& filter : filters) {
        current = applyFilterSpec(current, filter);
    }
    // Written last, so a bad filter can't damage an existing output file.
    saveImage(current, output);
    std::cout << "Wrote " << output << " from " << loaded.getLabel() << "\n";
}

int run(int argc, char** argv) {
    CLI::App app{"Image filter tool and C++ lifecycle demos"};

    int lab = 0;
    CLI::App* demo = app.add_subcommand("demo", "Show why each graded C++ concept is needed");
    demo->add_option("--lab", lab, "Run one lab instead of all of them")->check(CLI::Range(3, 5));

    std::string input;
    std::string output;
    std::vector<std::string> filters;
    CLI::App* filter = app.add_subcommand("filter", "Apply filters to an image, in order");
    filter->add_option("-i,--input", input, "Image to read")->required();
    filter->add_option("-o,--output", output, "Image to write: .png, .jpg, .jpeg or .bmp")
        ->required();
    filter->add_option("-f,--filter", filters, "grayscale, invert or blur:<radius>")->required();

    app.require_subcommand(1);
    if (argc == 1) {
        std::cout << app.help();
        return 1;
    }
    CLI11_PARSE(app, argc, argv);

    if (demo->parsed()) {
        runDemos(lab);
    } else if (filter->parsed()) {
        runFilters(input, output, filters);
    }
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    try {
        return run(argc, argv);
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << "\n";
        return 1;
    }
}
