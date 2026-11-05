#include "DemoTypes.hpp"

int orderDynamicValue() noexcept {
    // volatile stops the compiler from turning orderGlobalValue into a constant,
    // which would hide the startup order this demo shows.
    static volatile int seed = orderExpectedValue;
    return seed;
}

int orderGlobalValue = orderDynamicValue();

int& orderLocalValue() noexcept {
    static int value = orderDynamicValue();
    return value;
}
