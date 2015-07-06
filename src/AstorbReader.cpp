/*
 * AstorbReader.cpp
 *
 *  Created on: 16 Mar 2015
 *      Author: erik
 */

#include "AstorbReader.h"

#include <random>
#include <fstream>
#include <sstream>

using std::vector;
using std::string;
using kep_toolbox::planet;
using std::istringstream;
namespace model {

vector<Asteroid*> AstorbReader::readAsteroids(double fraction = 1) {

	vector<Asteroid*> asteroids;

	std::ifstream inStream(filename);

	string line;
	while (std::getline(inStream, line)) {

		if (static_cast<double>(random()) / static_cast<double>(RAND_MAX) <= fraction) {
			string name = line.substr(0,26);
			string asteroidClass = line.substr(65, 5);

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

	}

	return asteroids;

}

AstorbReader::~AstorbReader() {
	// TODO Auto-generated destructor stub
}

} /* namespace model */
