/*
 * Lambert.h
 *
 *  Created on: 14 Mar 2015
 *      Author: erik
 */

#ifndef LAMBERT_H_
#define LAMBERT_H_

#include "../pykep/src/keplerian_toolbox.h"
namespace model {

namespace lambert {

	using kep_toolbox::array3D;
	using kep_toolbox::planet;

	double calculateDeltaV(const planet* from, const planet* to, double tod, double tof);
}

} /* namespace model */

#endif /* LAMBERT_H_ */
