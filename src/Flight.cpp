/*
 * Flight.cpp
 *
 *  Created on: 13 Mar 2015
 *      Author: erik
 */

#include "Flight.h"

#include "Lambert.h"

#include <cmath>
#include <exception>

namespace model {

double Flight::getDeltaV(const planet* from, const planet* to) {
	// if delta v has been calculated return it
	if (deltaV >= 0) {
		return deltaV;
	}

	deltaV = lambert::calculateDeltaV(from, to, tod, tof);
	return deltaV;

}

} /* namespace model */
