#pragma once

namespace genericfsim::util {

#ifndef _DEBUG
constexpr bool RUN_IN_PARALLEL = true;
#define RELEASE_MODE
#else
constexpr bool RUN_IN_PARALLEL = false;
#define DEBUG_MODE
#endif

}
