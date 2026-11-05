#include <CLI/CLI.hpp>

int main(int argc, char** argv) {
    CLI::App app{"Image filter tool and C++ lifecycle demos"};
    CLI11_PARSE(app, argc, argv);
    return 0;
}
