#include "Logger.hpp"

#include <catch2/catch_test_macros.hpp>

#include <iostream>
#include <sstream>
#include <type_traits>

static_assert(!std::is_copy_constructible<Logger>::value, "Logger must not be copyable");
static_assert(!std::is_copy_assignable<Logger>::value, "Logger must not be assignable");

namespace {

// Captures std::cout, and leaves the Logger disabled for the next test.
class CoutCapture {
public:
    CoutCapture() : previous(std::cout.rdbuf(captured.rdbuf())) {}
    ~CoutCapture() {
        std::cout.rdbuf(previous);
        Logger::instance().setEnabled(false);
    }
    std::string text() const { return captured.str(); }

private:
    std::ostringstream captured;
    std::streambuf* previous;
};

} // namespace

TEST_CASE("Logger is one instance") {
    REQUIRE(&Logger::instance() == &Logger::instance());
}

TEST_CASE("Logger is silent until enabled") {
    CoutCapture capture;
    Logger::instance().trace("hidden");
    REQUIRE(capture.text().empty());
}

TEST_CASE("Logger prints when enabled") {
    CoutCapture capture;
    Logger::instance().setEnabled(true);
    Logger::instance().trace("visible");
    REQUIRE(capture.text().find("visible") != std::string::npos);
}
