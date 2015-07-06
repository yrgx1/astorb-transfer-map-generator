/*
 * FilterAsteroids.h
 *
 *  Created on: 18 Mar 2015
 *      Author: erik
 */

#ifndef FILTERASTEROIDS_H_
#define FILTERASTEROIDS_H_

#include "Asteroid.h"

#include <vector>
#include <string>

namespace model {

class FilterAsteroids {
public:
	static const double MINIMUM_SEMI_MAJOR_AXIS;
	static const double MAXIMUM_SEMI_MAJOR_AXIS;
	static const double MAXIMUM_INCLINATION;


	FilterAsteroids();
	void run(Asteroid* origin, std::string inputFilename, std::string outputFilename, bool findValues);
	virtual ~FilterAsteroids();

private:

	double findMinimumSemiMajorAxis(std::vector<Asteroid*> asteroids, Asteroid* origin);
	double findMaximumSemiMajorAxis(std::vector<Asteroid*> asteroids, Asteroid* origin);
	double findMaximumInclination(std::vector<Asteroid*> asteroids, Asteroid* origin);

	std::vector<Asteroid*> read(std::string filename);
	void filter(std::string inputFilename, std::string outputFilename, std::vector<Asteroid*> asteroids);
};

} /* namespace model */

#endif /* FILTERASTEROIDS_H_ */
