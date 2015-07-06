/*
 * AstorbReader.h
 *
 *  Created on: 16 Mar 2015
 *      Author: erik
 */

#ifndef ASTORBREADER_H_
#define ASTORBREADER_H_

#include "Asteroid.h"

#include <string>
#include <vector>

namespace model {

class AstorbReader {
public:
	AstorbReader(std::string filename) : filename(filename) {};
	std::vector<Asteroid*> readAsteroids(double fraction);

	virtual ~AstorbReader();
private:
	std::string filename;
};

} /* namespace model */

#endif /* ASTORBREADER_H_ */
