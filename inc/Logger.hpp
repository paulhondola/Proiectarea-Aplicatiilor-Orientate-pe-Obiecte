#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <string>

// The program's one lifecycle log. Reached through a function-local static so
// that it is safe to use from any source file, even during startup (Item 4).
class Logger {
public:
    static Logger& instance();

    // Only one Logger should ever exist (Item 6).
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    void setEnabled(bool enabled);
    void trace(const std::string& message) const;

private:
    Logger();

    bool enabled;
};

#endif // LOGGER_HPP
