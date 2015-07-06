/*
 * Flight.h
 *
 *  Created on: 13 Mar 2015
 *      Author: erik
 */

#ifndef FLIGHT_H_
#define FLIGHT_H_

#include "../pykep/src/keplerian_toolbox.h"


#include <limits>

namespace model {

using kep_toolbox::epoch;
using kep_toolbox::planet;
using kep_toolbox::array3D;

class Flight {
public:

	Flight(const epoch tod, double tof) {
		this->tod = tod.mjd2000();
		this->tof = tof;
	};

	Flight(double tod, double tof) {
		this->tod = tod;
		this->tof = tof;
	};

	Flight(const Flight& other) {
		this->tod = other.tod;
		this->tof = other.tof;
		this->deltaV = other.deltaV;
	};

	const epoch getEpoch() const {
		return tod;
	};

	double getTod() const {
		return tod;
	};
	double getTof() const {
		return tof;
	};

	void invalidate() {
		deltaV = +std::numeric_limits<double>::max();
	}
	const epoch getArrivalDate() const {
		return epoch(tod + tof);
	};

	Flight operator+(const Flight& other) {
		return Flight(this->tod + other.tod, this->tof + other.tof);
	};
	Flight operator-(const Flight& other) {
		return Flight(this->tod - other.tod, this->tof - other.tof);

	};

	Flight operator*(double constant) {
		return Flight(this->tod * constant, this->tof * constant);
	};

	Flight operator/(double constant) {
		return Flight(this->tod / constant, this->tof / constant);
	};

	Flight& operator=(const Flight& other) {
		this->tod = other.tod;
		this->tof = other.tof;
		this->deltaV = other.deltaV;
		return *this;
	};

	Flight& operator+=(const Flight& other) {
		this->tod += other.tod;
		this->tof += other.tof;
		deltaV = -1;
		return *this;
	};
	Flight& operator-=(const Flight& other) {
		this->tod -= other.tod;
		this->tof -= other.tof;
		deltaV = -1;

		return *this;
	};

	Flight& operator*=(const Flight& other) {
		this->tod *= other.tod;
		this->tof *= other.tof;
		deltaV = -1;

		return *this;
	};

	Flight& operator/=(const Flight& other) {
		this->tod /= other.tod;
		this->tof /= other.tof;
		deltaV = -1;

		return *this;
	};

	bool operator <(const Flight& other) const {
		if (this->deltaV < 0 or other.deltaV < 0) {
			throw std::logic_error("Cannot compare flights before calling calculateDeltaV");
		}

		return this->deltaV < other.deltaV;
	};
	bool operator >(const Flight& other) const {
		if (this->deltaV < 0 or other.deltaV < 0) {
			throw std::logic_error("Cannot compare flights before calling calculateDeltaV");
		}

		return this->deltaV > other.deltaV;
	};
	bool operator <=(const Flight& other) const {
		if (this->deltaV < 0 or other.deltaV < 0) {
			throw std::logic_error("Cannot compare flights before calling calculateDeltaV");
		}

		return this->deltaV <= other.deltaV;
	};
	bool operator >=(const Flight& other) const {
		if (this->deltaV < 0 or other.deltaV < 0) {
			throw std::logic_error("Cannot compare flights before calling calculateDeltaV");
		}

		return this->deltaV >= other.deltaV;
	};

	bool operator <(double value) const {
		if (this->deltaV < 0) {
			throw std::logic_error("Cannot compare flights before calling calculateDeltaV");
		}

		return this->deltaV < value;
	};
	bool operator >(double value) const {
		if (this->deltaV < 0) {
			throw std::logic_error("Cannot compare flights before calling calculateDeltaV");
		}

		return this->deltaV > value;
	};
	bool operator <=(double value) const {
		if (this->deltaV < 0) {
			throw std::logic_error("Cannot compare flights before calling calculateDeltaV");
		}

		return this->deltaV <= value;
	};
	bool operator >=(double value) const {
		if (this->deltaV < 0) {
			throw std::logic_error("Cannot compare flights before calling calculateDeltaV");
		}

		return this->deltaV >= value;
	};


	double getDeltaV(const planet* from, const planet* to);


	virtual ~Flight() {

	}
private:
	double tod;
	double tof;
	double deltaV = -1;

};

} /* namespace model */

#endif /* FLIGHT_H_ */
