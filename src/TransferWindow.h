/*
 * TransferWindow.h
 *
 *  Created on: 1 Jul 2015
 *      Author: erik
 */

#ifndef TRANSFERWINDOW_H_
#define TRANSFERWINDOW_H_

namespace model {

class Asteroid;

class TransferWindow {
public:
	short deltaV;
	short tod;
	short tof;
	Asteroid* target;

	TransferWindow(short deltaV, short tod, short tof, Asteroid* target) {
		this->deltaV = deltaV;
		this->tod = tod;
		this->tof = tof;
		this->target = target;
	}

};

} /* namespace model */

#endif /* TRANSFERWINDOW_H_ */
