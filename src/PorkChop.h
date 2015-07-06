/*
 * PorkChop.h
 *
 *  Created on: 14 Mar 2015
 *      Author: erik
 */

#ifndef PORKCHOP_H_
#define PORKCHOP_H_

#include "../pykep/src/keplerian_toolbox.h"

#include <string>

namespace model {

using kep_toolbox::planet;
using kep_toolbox::epoch;

class PorkChop {
public:
	PorkChop(const planet& asteroid1, const planet& asteroid2, int dimx, int dimy, epoch missionStart, epoch missionEnd, int maxTof, int maxDeltaV) :
		asteroid1(asteroid1), asteroid2(asteroid2), dimx(dimx), dimy(dimy), missionStart(missionStart), missionEnd(missionEnd), maxDeltaV(maxDeltaV) {
		if (maxTof < 0) {
			this->maxTof = static_cast<int>(missionEnd.mjd2000() - missionStart.mjd2000());
		} else {
			this->maxTof = maxTof;
		}
	}

	void generateTof(std::string filename);
	void generateArrival(std::string filename, epoch maximumTod, epoch minimumToa);
	virtual ~PorkChop();

private:
	planet asteroid1, asteroid2;
	int dimx, dimy;
	epoch missionStart, missionEnd;
	int maxTof;
	int maxDeltaV;
};

} /* namespace model */

#endif /* PORKCHOP_H_ */
