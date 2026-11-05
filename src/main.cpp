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
    std::cout << "\nAll checks passed.\n";
}

} // namespace

int main(int argc, char** argv) {
    CLI::App app{"Image filter tool and C++ lifecycle demos"};

    int lab = 0;
    CLI::App* demo = app.add_subcommand("demo", "Show why each graded C++ concept is needed");
    demo->add_option("--lab", lab, "Run one lab instead of all of them")->check(CLI::Range(3, 3));

    if (argc == 1) {
        std::cout << app.help();
        return 1;
    }
    CLI11_PARSE(app, argc, argv);

    try {
        if (demo->parsed()) {
            runDemos(lab);
        }
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << "\n";
        return 1;
    }
    return 0;
}
