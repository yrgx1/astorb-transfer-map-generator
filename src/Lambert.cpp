/*
 * Lambert.cpp
 *
 *  Created on: 14 Mar 2015
 *      Author: erik
 */

#include "Lambert.h"

#include <cmath>

namespace model {

namespace lambert {

array3D array3D_diff(array3D a, array3D b) {
	kep_toolbox::array3D diff;
	diff = a;
	diff[0] -= b[0];
	diff[1] -= b[1];
	diff[2] -= b[2];
	return diff;
}

/**
 * @brief Takes r0 and r1 (unit of measurement is AU), tof (time of flight, number of seconds from departure to arrival) and v0/v1 (in m/s)
 */
void lambertMethod(const array3D &r0, const array3D &r1, double tof, array3D &v0, array3D &v1) {
	static const double mu = ASTRO_MU_SUN;
	static const int cw = false;
	static const int multirevs = 0;

	kep_toolbox::lambert_problem l(r0, r1, tof, mu, cw, multirevs);

	v0 = l.get_v1()[0];
	v1 = l.get_v2()[0];


}

double calculateDeltaV(const planet* from, const planet* to, double tod, double tof) {
	using std::sqrt;
	using std::pow;

	array3D r0, r1, v0, v1;

	from->get_eph(tod, r0, v0);
	to->get_eph(tod+tof, r1, v1);


	double tof_seconds = tof * ASTRO_DAY2SEC;

	array3D tv1, tv2;
	lambertMethod(r0, r1, tof_seconds, tv1, tv2);

	array3D dv1 = array3D_diff(tv1, v0);
	array3D dv2 = array3D_diff(v1, tv2);

	double deltaV = sqrt( pow(dv1[0],2) + pow(dv1[1],2) + pow(dv1[2],2)) + sqrt( pow(dv2[0],2) + pow(dv2[1],2) + pow(dv2[2],2));

	return deltaV;

}


} /* namespace lambert */
} /* namespace model */
