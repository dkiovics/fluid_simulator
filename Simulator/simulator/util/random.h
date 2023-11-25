#pragma once

#include <cstdlib>

namespace genericfsim::util
{

/**
 * Resturns a semi-random double in the range [0, 1].
 * 
 * \return - a random double
 */
inline double get() {
	return double(std::rand()) / RAND_MAX;
}

/**
 * Returns a semi-random double in the specified range.
 * 
 * \param min - inclusive
 * \param max - inclusive
 * \return - random double
 */
inline double getDoubleInRange(double min, double max) {
	return min + (max - min) * get();
}

/**
 * Returns a semi-random int in the specified range.
 *
 * \param min - inclusive
 * \param max - exclusive
 * \return - random double
 */
inline int getIntInRange(int min, int max) {
	return min + (max - min) * (double(std::rand()) / (RAND_MAX + 1));
}

}
