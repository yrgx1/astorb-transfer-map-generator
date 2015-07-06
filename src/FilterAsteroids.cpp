/*
 * FilterAsteroids.cpp
 *
 *  Created on: 18 Mar 2015
 *      Author: erik
 */

#include "FilterAsteroids.h"
#include "utility"
#include "NelderMead.h"

#include "../pykep/src/keplerian_toolbox.h"

#include <algorithm>
#include <iostream>
#include <fstream>
#include <omp.h>

namespace model {

const double FilterAsteroids::MINIMUM_SEMI_MAJOR_AXIS = 3.28533e+11;
const double FilterAsteroids::MAXIMUM_SEMI_MAJOR_AXIS = 4.79492e+11;
const double FilterAsteroids::MAXIMUM_INCLINATION = 0.0882419;

FilterAsteroids::FilterAsteroids() {
	// TODO Auto-generated constructor stub

}

std::vector<Asteroid*> FilterAsteroids::read(std::string filename) {

	using std::vector;
	using std::string;
	using std::istringstream;

	vector<Asteroid*> asteroids;

	std::ifstream inStream(filename);

	string line;
	while (std::getline(inStream, line)) {

		string name = line.substr(0,26);
		// ignore discoverer, magnitude, slope parameter, color index, diameter
		string asteroidClass = line.substr(65, 5);
		// ignore planet crossing

		// throw away if orbit is unknown
		int orbitComputation;
		istringstream(line.substr(75, 3)) >> orbitComputation;
		if (orbitComputation != 0) {
			continue;
		}

		// ignore major survey

		// throw away if rarely observed
		int criticalListNumbered;
		istringstream(line.substr(85,1)) >> criticalListNumbered;
		if (criticalListNumbered != 0 and criticalListNumbered != 7) {
			continue;
		}

		// ignore Lowell observatory, rank (of importance to observe from Earth)
		// ignore orbital arcs, numbers of observations, epoch of osculation

		double semiMajorAxis, eccentricity, inclination, longitudeOfAscendingNode, argumentOfPeriapsis, meanAnomaly;
		istringstream(line.substr(115,10)) >> meanAnomaly;
		istringstream(line.substr(126,10)) >> argumentOfPeriapsis;
		istringstream(line.substr(137,10)) >> longitudeOfAscendingNode;
		istringstream(line.substr(148,9)) >> inclination;
		istringstream(line.substr(158,10)) >> eccentricity;
		istringstream(line.substr(170,11)) >> semiMajorAxis;


		Asteroid* asteroid = new Asteroid(name, asteroidClass, semiMajorAxis, eccentricity, inclination, longitudeOfAscendingNode, argumentOfPeriapsis, meanAnomaly);
		asteroids.push_back(asteroid);

	}

	return asteroids;
}

void FilterAsteroids::filter(std::string inputFilename, std::string outputFilename, std::vector<Asteroid*> asteroids) {

	using std::vector;
	using std::string;
	using std::istringstream;

	std::ifstream inStream(inputFilename);
	std::ofstream outStream(outputFilename);

	string line;
	auto iterator = asteroids.begin();

	while (std::getline(inStream, line)) {
		string name = line.substr(0,26);

		if (iterator != asteroids.end()) {
			if (name == (*iterator)->getName()) {
				outStream << line << std::endl;
				iterator++;
			}
		} else {
			break;
		}
	}

	inStream.close();
	outStream.close();
}


double FilterAsteroids::findMinimumSemiMajorAxis(std::vector<Asteroid*> asteroids, Asteroid* origin) {

	std::vector<std::pair<double,Asteroid*>> pairs;
	for (Asteroid* asteroid : asteroids) {
		kep_toolbox::array6D orbitalElements = asteroid->getPlanet().get_elements();
		pairs.push_back(std::make_pair(orbitalElements[0], asteroid));
	}
	std::sort(pairs.begin(), pairs.end());

	// finding lowest semi-major axis with transfer window
	double before = getMillis();
	double lowest = 1E200;

	unsigned int finishedIndex = 1E8;
	#pragma omp parallel
	{
		int id = omp_get_thread_num();
		NelderMead nm;
		kep_toolbox::planet originCopy = origin->getPlanet();

		unsigned int i=id;
		while (i < finishedIndex and i < pairs.size()) {

			double value = pairs[i].first;
			Asteroid* asteroid = pairs[i].second;

			unsigned int printAt = 1000;
			if (i % printAt == 0) {
				#pragma omp critical
				std::cout << "Iteration " << i/1000 << "k:\t semiMajor=" << value << std::endl;
			}

			std::vector<Flight> minima = nm.calculateMinima(&originCopy, &asteroid->getPlanet());

			if (minima.size() > 0)
			#pragma omp critical
			{
					if (value < lowest) {
						lowest = value;
					}

					finishedIndex = i;
			}
			i += omp_get_num_threads();
		}
	}
	double after = getMillis();

	std::cout << "Lowest Semi-Major Axis with feasible transfer windows found is " << lowest << " (took " << after -before << " ms)." << std::endl;

	return lowest;
}

double FilterAsteroids::findMaximumSemiMajorAxis(std::vector<Asteroid*> asteroids, Asteroid* origin) {

	std::vector<std::pair<double,Asteroid*>> pairs;
	for (Asteroid* asteroid : asteroids) {
		kep_toolbox::array6D orbitalElements = asteroid->getPlanet().get_elements();
		pairs.push_back(std::make_pair(orbitalElements[0], asteroid));
	}
	std::sort(pairs.rbegin(), pairs.rend());

	// finding highest semi-major axis with transfer window
	double before = getMillis();
	double highest = -1E200;

	unsigned int finishedIndex = 1E8;
	#pragma omp parallel
	{
		int id = omp_get_thread_num();
		NelderMead nm;
		kep_toolbox::planet originCopy = origin->getPlanet();

		unsigned int i=id;
		while (i < finishedIndex and i < pairs.size()) {

			double value = pairs[i].first;
			Asteroid* asteroid = pairs[i].second;

			unsigned int printAt = 1000;
			if (i % printAt == 0) {
				#pragma omp critical
				std::cout << "Iteration " << i/1000 << "k:\t semiMajor=" << value << std::endl;
			}

			std::vector<Flight> minima = nm.calculateMinima(&originCopy, &asteroid->getPlanet());

			if (minima.size() > 0)
			#pragma omp critical
			{
					if (value > highest) {
						highest = value;
					}

					finishedIndex = i;
			}
			i += omp_get_num_threads();
		}
	}
	double after = getMillis();

	std::cout << "Highest Semi-Major Axis with feasible transfer windows found is " << highest << " (took " << after -before << " ms)." << std::endl;

	return highest;
}

double FilterAsteroids::findMaximumInclination(std::vector<Asteroid*> asteroids, Asteroid* origin) {

	std::vector<std::pair<double,Asteroid*>> pairs;
	for (Asteroid* asteroid : asteroids) {
		kep_toolbox::array6D orbitalElements = asteroid->getPlanet().get_elements();
		pairs.push_back(std::make_pair(orbitalElements[2], asteroid));
	}
	std::sort(pairs.rbegin(), pairs.rend());

	// finding highest semi-major axis with transfer window
	double before = getMillis();
	double highest = -1;

	unsigned int finishedIndex = 1E8;
	#pragma omp parallel
	{
		int id = omp_get_thread_num();
		NelderMead nm;
		kep_toolbox::planet originCopy = origin->getPlanet();

		unsigned int i=id;
		while (i < finishedIndex and i < pairs.size()) {

			double value = pairs[i].first;
			Asteroid* asteroid = pairs[i].second;

			unsigned int printAt = 1000;
			if (i % printAt == 0) {
				#pragma omp critical
				std::cout << "Iteration " << i/1000 << "k:\t inclination=" << value << std::endl;
			}

			std::vector<Flight> minima = nm.calculateMinima(&originCopy, &asteroid->getPlanet());

			if (minima.size() > 0)
			#pragma omp critical
			{
					if (value > highest) {
						highest = value;
					}

					finishedIndex = i;
			}
			i += omp_get_num_threads();
		}
	}
	double after = getMillis();

	std::cout << "Highest Inclination with feasible transfer windows found is " << highest << " (took " << after -before << " ms)." << std::endl;

	return highest;
}

void FilterAsteroids::run(Asteroid* origin, std::string inputFilename, std::string outputFilename, bool findValues) {
	using std::vector;

	double before, after;
	std::vector<Asteroid*> newAsteroids;

	std::cout << "FilterAsteroids: loading input file...";
	std::cout.flush();

	before = getMillis();
	vector<Asteroid*> asteroids = read(inputFilename);
	after = getMillis();
	std::cout << " " << asteroids.size() << " loaded in " << after - before << " ms." << std::endl << std::endl;





	// reducing according to minimum semi major axis
	double minSemiMajor = MINIMUM_SEMI_MAJOR_AXIS;
	if (findValues) {
		minSemiMajor = findMinimumSemiMajorAxis(asteroids, origin);
	}

	newAsteroids.clear();
	for (Asteroid* asteroid : asteroids) {
		if (asteroid->getPlanet().get_elements()[0] > minSemiMajor) {
			newAsteroids.push_back(asteroid);
		} else {
			delete asteroid;
		}
	}
	asteroids = newAsteroids;
	std::cout << "Remaining after limiting minimum Semi-Major Axis: " << asteroids.size() << "." << std::endl << std::endl;



	// reducing according to maximum semi major axis
	double maxSemiMajor = MAXIMUM_SEMI_MAJOR_AXIS;
	if (findValues) {
		maxSemiMajor = findMaximumSemiMajorAxis(asteroids, origin);
	}

	newAsteroids.clear();
	for (Asteroid* asteroid : asteroids) {
		if (asteroid->getPlanet().get_elements()[0] < maxSemiMajor) {
			newAsteroids.push_back(asteroid);
		} else {
			delete asteroid;
		}
	}
	asteroids = newAsteroids;
	std::cout << "Remaining after limiting maximum Semi-Major Axis: " << asteroids.size() << "." << std::endl << std::endl;



	// reducing according to maximum inclination
	double maxInclination = MAXIMUM_INCLINATION;
	if (findValues) {
		maxInclination = findMaximumInclination(asteroids, origin);
	}

	newAsteroids.clear();
	for (Asteroid* asteroid : asteroids) {
		if (asteroid->getPlanet().get_elements()[2] < maxInclination) {
			newAsteroids.push_back(asteroid);
		} else {
			delete asteroid;
		}
	}
	asteroids = newAsteroids;
	std::cout << "Remaining after limiting maximum Inclination: " << asteroids.size() << "." << std::endl << std::endl;


	filter(inputFilename, outputFilename, asteroids);



}

FilterAsteroids::~FilterAsteroids() {
	// TODO Auto-generated destructor stub
}

} /* namespace model */
