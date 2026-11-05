#ifndef DEMOS_HPP
#define DEMOS_HPP

#include <iostream>
#include <stdexcept>
#include <string>

// Always runs, unlike assert: a demo must behave the same in Release.
// Descriptions must not contain addresses or timings, because
// `scripts/verify.sh parity` compares them across builds.
inline void check(bool condition, const std::string& description) {
    if (!condition) {
        throw std::runtime_error("Check failed: " + description);
    }
    std::cout << "[PASS] " << description << "\n";
}

inline void printHeading(const std::string& title) {
    std::cout << "\n--- " << title << " ---\n";
}

// Counts what assigning in a constructor body costs against an initializer list. Lucrarea 3
// and Item 4 both show it.
void compareInitializationCost();

void runLab3();
void runLab4();
void runLab5();

#endif // DEMOS_HPP
