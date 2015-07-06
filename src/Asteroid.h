/*
 * Asteroid.h
 *
 *  Created on: 16 Mar 2015
 *      Author: erik
 */

#ifndef ASTEROID_H_
#define ASTEROID_H_

#include "../pykep/src/keplerian_toolbox.h"

#include <string>
#include <vector>
#include <deque>
#include "Transferbase.h"
#include "TransferWindow.h"

namespace model {

class Asteroid {
public:

	// angles in degrees
	Asteroid(std::string name, std::string asteroidClass, double semiMajorAxis, double eccentricity, double inclination, double longitudeOfAscendingNode, double argumentOfPeriapsis, double meanAnomaly);

	void calculateTransfers(const std::vector<Asteroid*>& database, Transferbase* transferbase, int maxDeltaV, int earliestArrival);


	std::string to_string();
	const kep_toolbox::planet& getPlanet() {
		return kep_planet;
	}
	const std::string& getName() {
		return name;
	}

	virtual ~Asteroid();

private:
	kep_toolbox::planet kep_planet;
	std::string name;
	std::string asteroidClass;

	void recursiveCall(const std::vector<Asteroid*>& database, Transferbase* transferbase, int maxDeltaV, int earliestArrival, int depth);
	int maxDeltaV = -1;
	int earliestArrival = 2E+9;
	std::deque<TransferWindow> transferWindows;
	size_t listOfTractableEnd = 0;


	static int calculatedCount;
};

} /* namespace model */

#endif /* ASTEROID_H_ */
