#include "Demos.hpp"
#include "Logger.hpp"

#include <CLI/CLI.hpp>

#include <exception>
#include <iostream>

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

int run(int argc, char** argv) {
    CLI::App app{"Image filter tool and C++ lifecycle demos"};

    int lab = 0;
    CLI::App* demo = app.add_subcommand("demo", "Show why each graded C++ concept is needed");
    demo->add_option("--lab", lab, "Run one lab instead of all of them")->check(CLI::Range(3, 5));

    if (argc == 1) {
        std::cout << app.help();
        return 1;
    }
    CLI11_PARSE(app, argc, argv);

    if (demo->parsed()) {
        runDemos(lab);
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
