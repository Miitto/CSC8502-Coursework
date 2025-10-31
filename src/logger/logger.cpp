#include "logger.hpp"

#ifndef NDEBUG
#define LEVEL trace
#else
#define LEVEL info
#endif

DEFINE_LOGGER("App", LEVEL)