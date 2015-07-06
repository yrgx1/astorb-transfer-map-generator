/*
 * NelderMead.h
 *
 *  Created on: 11 Mar 2015
 *      Author: erik
 */

#ifndef NELDERMEAD_H_
#define NELDERMEAD_H_

#include "../pykep/src/keplerian_toolbox.h"
#include "Flight.h"

#include <vector>

namespace model {

using kep_toolbox::epoch;
using kep_toolbox::planet;
using kep_toolbox::array3D;

#define TRIANGLE_OPTIMIZATION true

#define NELDER_MEAD_DEBUG false

class NelderMead {
public:

	std::vector<Flight> calculateMinima(const planet* from, const planet* to);



	virtual ~NelderMead() {
	}

	static double getEpsilon() {
		return epsilon;
	}

	static void setEpsilon(double epsilon) {
		NelderMead::epsilon = epsilon;
	}

	static double getMissionDuration() {
		return missionDuration;
	}

	static void setMissionDuration(double missionDuration) {
		NelderMead::missionDuration = missionDuration;
	}

	static double getMaximumTof() {
			return maximumTof;
	}

	static void setMaximumTof(double maxTof) {
		NelderMead::maximumTof = maxTof;
	}

	static double getMissionStart() {
		return missionStart;
	}

	static void setMissionStart(epoch missionStart) {
		NelderMead::missionStart = missionStart.mjd2000();
	}

	static void setMissionStart(double missionStart) {
		NelderMead::missionStart = missionStart;
	}

	static int getMaxIterations() {
		return maxIterations;
	}

	static void setMaxIterations(int maxIterations = 100) {
		NelderMead::maxIterations = maxIterations;
	}

	static int getResolution() {
		return resolution;
	}

	static void setResolution(int resolution = 15) {
		NelderMead::resolution = resolution;
	}

	static double getMaximumDeltaV() {
		return maximumDeltaV;
	}

	static void setMaximumDeltaV(int maximumDeltaV) {
		NelderMead::maximumDeltaV = maximumDeltaV;
	}

#if NELDER_MEAD_DEBUG
	static void printDebug();
#endif

private:

	struct Triangle {
		int d,t;
		bool flipped;
	};

	void calculateDeltaV(Flight& flight);
	Triangle nelderMeadReflection(Triangle triangle, std::vector<std::vector<Flight>>& gridFlights );
	Flight calculateMinimum(const Flight& f1, const Flight& f2, const Flight& f3);
	bool reflectTriangle(NelderMead::Triangle& triangle, std::vector<std::vector<Flight>>& gridFlights, int dd, int dt, double worstdv);

	static double epsilon;
	static double missionStart;
	static double missionDuration;
	static double maximumTof;
	static int resolution;
	static int maxIterations;
	static int maximumDeltaV;

#if NELDER_MEAD_DEBUG
	static long trianglesCreated;
	static long trianglesRemoved;
	static long trianglesReflected;
#endif

	const planet* from;
	const planet* to;
};

} /* namespace model */

#endif /* NELDERMEAD_H_ */
