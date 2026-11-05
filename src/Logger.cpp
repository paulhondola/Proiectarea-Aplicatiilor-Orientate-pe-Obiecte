#include "Logger.hpp"

#include <iostream>

Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

Logger::Logger() : enabled(false) {
}

void Logger::setEnabled(bool newEnabled) {
    enabled = newEnabled;
}

void Logger::trace(const std::string& message) const {
    if (enabled) {
        std::cout << "  " << message << "\n";
    }
}
