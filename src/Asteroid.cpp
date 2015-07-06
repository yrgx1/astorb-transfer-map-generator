/*
 * Asteroid.cpp
 *
 *  Created on: 16 Mar 2015
 *      Author: erik
 */

#include "Asteroid.h"

#include "NelderMead.h"
#include "Flight.h"

#include <sstream>
#include <unordered_map>
#include <algorithm>

#include <iostream>

namespace model {


int Asteroid::calculatedCount = 0;

Asteroid::Asteroid(std::string name, std::string asteroidClass, double semiMajorAxis, double eccentricity, double inclination,
		double longitudeOfAscendingNode, double argumentOfPeriapsis, double meanAnomaly) :
											name(name), asteroidClass(asteroidClass) {

	kep_toolbox::array6D orbitalElements = {
			semiMajorAxis * ASTRO_AU,
			eccentricity,
			inclination * ASTRO_DEG2RAD,
			longitudeOfAscendingNode * ASTRO_DEG2RAD,
			argumentOfPeriapsis * ASTRO_DEG2RAD,
			meanAnomaly * ASTRO_DEG2RAD
	};
	kep_planet = kep_toolbox::planet(0,orbitalElements,ASTRO_MU_SUN);
}

std::string Asteroid::to_string() {
	std::stringstream stream;
	stream << name << " " << asteroidClass;
	return stream.str();
}

void Asteroid::calculateTransfers(const std::vector<Asteroid*>& database, Transferbase* transferbase, int maxDeltaV, int earliestArrival) {
	recursiveCall(database, transferbase, maxDeltaV, earliestArrival, 1);
}


void Asteroid::recursiveCall(const std::vector<Asteroid*>& database, Transferbase* transferbase, int deltaV, int arrival, int depth) {
	if (this->maxDeltaV == -1) {
		size_t asteroidCount = 0;
		// check if asteroid has been calculated before

		if (transferbase->lookup(this)) {
			std::cout << 'r';
			// do not calculate - only retrieve
			transferWindows = transferbase->getTransferWindows(this);

			// calculate number of unique asteroids
			Asteroid* current = nullptr;
			for (TransferWindow tw : transferWindows) {
				if (tw.target != current) {
					current = tw.target;
					++asteroidCount;
				}
			}
		} else {
			// must calculate and write
			std::cout << 'c';
#pragma omp parallel
			{
				// planet and NelderMead are not thread safe --> one copy for each thread
				planet thisPlanetCopy = this->getPlanet();
				NelderMead nm;

#pragma omp for schedule(dynamic)
				for (unsigned int i=0; i < database.size(); ++i) {
					Asteroid* asteroid = database[i];
					if (asteroid != this) {

						// calculate transfers and add to transferbase
						std::vector<Flight> minima = nm.calculateMinima(&thisPlanetCopy, &asteroid->getPlanet());

						if (minima.size() > 0)
#pragma omp critical
						{
							for (Flight &flight : minima) {
								TransferWindow transfer (
										static_cast<short>(flight.getDeltaV(nullptr, nullptr)),
										static_cast<short>(flight.getTod()),
										static_cast<short>(flight.getTof()),
										asteroid
								);
								transferWindows.push_back(transfer);
							}
							++asteroidCount;
						}
					}
				}
			}

			// write to file
			transferbase->write(this, transferWindows);
		}

		std::sort(transferWindows.begin(), transferWindows.end(), [](TransferWindow a, TransferWindow b){return a.tod > b.tod;});

		std::cout << ++calculatedCount;
		for (int i=0; i < depth-1; ++i) {
			std::cout << "    ";
		}
		std::cout << "[" << to_string() << "]" << "\treachable=" << asteroidCount << "\t\ttransfers=" << transferWindows.size();

	}

	bool betterDeltaV = (this->maxDeltaV < deltaV);
	bool betterTime = (this->earliestArrival > arrival);


	if (betterDeltaV or betterTime) {

		for (int i=0; i < depth; ++i) {
			std::cout << "    ";
		}
		if (this->maxDeltaV != -1) {
			std::cout << "[" << to_string() << "]";
		}
		if (betterDeltaV) {
			this->maxDeltaV = deltaV;
			std::cout << "\tdeltaV=" << deltaV;
		}
		if (betterTime) {
			this->earliestArrival = arrival;
			std::cout << "\tarrival=" << arrival;
		}
		std::cout << std::endl;

		for (TransferWindow window : transferWindows) {
			if (window.tod >= arrival) {
				int remainingDeltaV = deltaV - window.deltaV;
				if (remainingDeltaV >= 0) {
					int toa = window.tod + window.tof;
					window.target->recursiveCall(database, transferbase, remainingDeltaV, toa, depth+1);
				} else {
					// others might be cheaper
					continue;
				}
			} else {
				// all remaining windows have tod >= this one's
				break;
			}
		}
	}
}

Asteroid::~Asteroid() {
	// TODO Auto-generated destructor stub
}

} /* namespace model */
