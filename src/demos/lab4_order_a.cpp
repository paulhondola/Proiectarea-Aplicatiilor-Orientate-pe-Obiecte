#include "DemoTypes.hpp"

// Both run during startup, in an order the standard leaves open across files.
// Reading orderGlobalValue early is well defined: an int is zero-initialized
// before any dynamic initialization runs, so the value is 0 or the real one.
int orderCopiedAtStartup = orderGlobalValue;

// The function-local static is initialized on first use, so order can't matter.
int orderLocalAtStartup = orderLocalValue();
