/*
 * NelderMead.cpp
 *
 *  Created on: 11 Mar 2015
 *      Author: erik
 */

#include "NelderMead.h"

#include <algorithm>
#include <cmath>

using std::vector;
namespace model {

double NelderMead::epsilon = 1;
double NelderMead::missionStart = 0;
double NelderMead::missionDuration = 0;
double NelderMead::maximumTof = 0;
int NelderMead::resolution = 15;
int NelderMead::maxIterations = 100;
int NelderMead::maximumDeltaV = 10000;

#if NELDER_MEAD_DEBUG

#include <iostream>

long NelderMead::trianglesCreated = 0;
long NelderMead::trianglesRemoved = 0;
long NelderMead::trianglesReflected = 0;

void NelderMead::printDebug() {
	std::cout << "NelderMead::trianglesCreated: " << NelderMead::trianglesCreated << std::endl;
	std::cout << "NelderMead::trianglesRemoved: " << NelderMead::trianglesRemoved << std::endl;
	std::cout << "NelderMead::trianglesReflected: " << NelderMead::trianglesReflected << std::endl;
}

#endif

std::vector<Flight> NelderMead::calculateMinima(const planet* from, const planet* to) {

	this->from = from;
	this->to = to;

	double iterationFraction  = missionDuration / resolution;
	int tofResolution = static_cast<int> (std::floor(static_cast<double>(resolution) * maximumTof / missionDuration));

	std::vector<Flight> minima;

	Flight baseline(missionStart,0);
	Flight componentTod(iterationFraction,0);
	Flight componentTof(0,iterationFraction);
	Flight invalidFlight(0,0);
	invalidFlight.invalidate();


	// generate grid
	std::vector<std::vector<Flight>> gridFlights;
	gridFlights.resize(resolution);
	for (int i=0; i<resolution; ++i) {
		gridFlights[i].resize(tofResolution+1, invalidFlight);
	}

	// fill grid
	for (int d=1; d < resolution; d++) {
		for (int t=1; t < std::min(resolution-d, tofResolution); t++) {
			Flight point = baseline;
			point += (componentTod * d);
			point += (componentTof * t);

			calculateDeltaV(point);
			gridFlights[d][t] = point;
		}
	}

#if TRIANGLE_OPTIMIZATION == true
	// find potential starting points in grid
	vector<Triangle> triangles;
	for (int d=1; d < resolution-1; d+=2) {
		for (int t=1; t < std::min(resolution-d, tofResolution); t+=3) {
			Triangle t1 = {d,t,false};

			Triangle reflected = nelderMeadReflection(t1, gridFlights);
			triangles.push_back(reflected);

			if (t+2 < std::min(resolution-d, tofResolution)) {
				Triangle t2 = {d+1, t+2, true};
				triangles.push_back(nelderMeadReflection(t2, gridFlights));
			}
		}
	}

	// eliminate duplicate starting points
	vector<Triangle> uniqueTriangles;
	for (int i=static_cast<int>(triangles.size())-1; i>=0; --i) {
		Triangle &ti = triangles[i];
		int j;
		for (j=0; j < i; j++) {
			Triangle &tj = triangles[j];
			if (ti.t == tj.t and ti.d == tj.d and ti.flipped == tj.flipped) {
				break;
			}
		}
		if (j == i) {
			uniqueTriangles.push_back(ti);
		}
	}

#if NELDER_MEAD_DEBUG
#pragma omp critical
	{
		trianglesCreated += triangles.size();
		trianglesRemoved += triangles.size() - uniqueTriangles.size();
	}
#endif
	// run full nelder mead on remaining triangles

	for (Triangle t : uniqueTriangles) {
		if (not t.flipped) {
			Flight& f1 = gridFlights[t.d][t.t];
			Flight& f2 = gridFlights[t.d][t.t+1];
			Flight& f3 = gridFlights[t.d+1][t.t];

			Flight minimum1 = calculateMinimum(f1, f2, f3);
			if (minimum1.getDeltaV(from, to) < maximumDeltaV) {
				minima.push_back(minimum1);
			}

		} else {
			Flight& f4 = gridFlights[t.d][t.t];
			Flight& f5 = gridFlights[t.d][t.t-1];
			Flight& f6 = gridFlights[t.d-1][t.t];

			Flight minimum2 = calculateMinimum(f4, f5, f6);
			if (minimum2.getDeltaV(from, to) < maximumDeltaV) {
				minima.push_back(minimum2);
			}

		}
	}
#else
	for (int d=1; d < resolution; d+=2) {
		for (int t=1; t < resolution-d; t+=3) {

			// find three points of first triangle
			Flight& f1 = gridFlights[d][t];
			Flight& f2 = gridFlights[d][t+1];
			Flight& f3 = gridFlights[d+1][t];

//			if (f1 < maximumDeltaV or f2 < maximumDeltaV or f3 < maximumDeltaV) {
				Flight minimum1 = calculateMinimum(f1, f2, f3);

				if (minimum1.getDeltaV(from, to) < maximumDeltaV) {
					minima.push_back(minimum1);
				}
//			}


			// find three points of second triangle if not outside solutino space
			if (t+2 < resolution-d) {
				Flight& f4 = gridFlights[d][t+2];
				Flight& f5 = gridFlights[d+1][t+2];
				Flight& f6 = gridFlights[d+1][t+1];

//				if (f4 < maximumDeltaV or f5 < maximumDeltaV or f6 < maximumDeltaV) {
					Flight minimum2 = calculateMinimum(f4, f5, f6);

					if (minimum2.getDeltaV(from, to) < maximumDeltaV) {
						minima.push_back(minimum2);
					}
//				}
			}
		}
	}

#endif
	// eliminate duplicates

	std::vector<bool> duplicateTransfer;
	duplicateTransfer.resize(minima.size(),false);

	// TODO: adjust
	double precision = 10;

	for (unsigned int i=0; i < minima.size(); i++) {
		if (duplicateTransfer[i]) {
			continue;
		}

		double xi = minima[i].getTod();
		double yi = minima[i].getTof();

		for (unsigned int k=i+1; k < minima.size(); k++) {
			if (duplicateTransfer[k]) {
				continue;
			}

			double xk = minima[k].getTod();
			double yk = minima[k].getTof();

			if (std::abs(xi-xk) < precision) {
				if (std::abs(yi-yk) < precision) {
					if (minima[i] <= minima[k]) {
						duplicateTransfer[k] = true;
						continue;
					} else {
						duplicateTransfer[i] = true;
						break;
					}
				} else if (yi <= yk and minima[i] <= minima[k]) {
					duplicateTransfer[k] = true;
					continue;
				} else if (yk <= yi and minima[k] <= minima[i]) {
					duplicateTransfer[i] = true;
					break;
				}
			}
		}
	}

	std::vector<Flight> nonDuplicates;

	for (unsigned int i=0; i < minima.size(); ++i) {
		if (not duplicateTransfer[i]) {
			nonDuplicates.push_back(minima[i]);
		}
	}

	std::sort(nonDuplicates.begin(), nonDuplicates.end());

	// return remainders
	return nonDuplicates;
}

bool NelderMead::reflectTriangle(NelderMead::Triangle& triangle, std::vector<std::vector<Flight>>& gridFlights, int dd, int dt, double worstdv) {
	double dvnew = gridFlights[triangle.d+dd][triangle.t+dt].getDeltaV(from, to);
#if NELDER_MEAD_DEBUG
#pragma omp critical
	trianglesReflected += 1;
#endif

	if (dvnew < worstdv) {
			triangle.d += dd;
			triangle.t += dt;
			triangle.flipped = not triangle.flipped;
			return true;
	} else {
		// new is even worse - contraction
		return false;
	}
}

NelderMead::Triangle NelderMead::nelderMeadReflection(NelderMead::Triangle triangle, std::vector<std::vector<Flight>>& gridFlights) {

	// terminate if convergence
	for (int iteration=0; iteration < maxIterations; iteration++) {

		int d = triangle.d, t = triangle.t;

		if (triangle.flipped) {
			double dv1 = gridFlights[d][t].getDeltaV(from, to);
			double dv2 = gridFlights[d][t-1].getDeltaV(from, to);
			double dv3 = gridFlights[d-1][t].getDeltaV(from, to);

			if (dv1 > dv2) {
				if (dv1 > dv3) {
					// 1 is the worst
					if (reflectTriangle(triangle, gridFlights, -1, -1, dv1)) {
						continue;
					} else {
						return triangle;
					}
				} else {
					// 3 is worst
					if (reflectTriangle(triangle, gridFlights, +1, -1, dv3)) {
						continue;
					} else {
						return triangle;
					}
				}
			} else {
				if (dv2 > dv3) {
					// 2 is the worst
					if (reflectTriangle(triangle, gridFlights, -1, +1, dv2)) {
						continue;
					} else {
						return triangle;
					}
				}
				else {
					// 3 is worst
					if (reflectTriangle(triangle, gridFlights, +1, -1, dv3)) {
						continue;
					} else {
						return triangle;
					}
				}
			}


		} else {
			double dv1 = gridFlights[d][t].getDeltaV(from, to);
			double dv2 = gridFlights[d][t+1].getDeltaV(from, to);
			double dv3 = gridFlights[d+1][t].getDeltaV(from, to);

			if (dv1 > dv2) {
				if (dv1 > dv3) {
					// 1 is the worst
					if (reflectTriangle(triangle, gridFlights, +1, +1, dv1)) {
						continue;
					} else {
						return triangle;
					}
				} else {
					// 3 is worst
					if (reflectTriangle(triangle, gridFlights, -1, +1, dv3)) {
						continue;
					} else {
						return triangle;
					}
				}
			} else {
				if (dv2 > dv3) {
					// 2 is the worst
					if (reflectTriangle(triangle, gridFlights, +1, -1, dv2)) {
						continue;
					} else {
						return triangle;
					}
				}
				else {
					// 3 is worst
					if (reflectTriangle(triangle, gridFlights, -1, +1, dv3)) {
						continue;
					} else {
						return triangle;
					}
				}
			}
		}

	}

	return triangle;
}

Flight NelderMead::calculateMinimum(const Flight& f1, const Flight& f2, const Flight& f3) {

	static const double r=1, c=-0.5, e=2, s=0.5; // reflection, contraction, expansion, shrinkage

	std::vector<Flight> flights;
	flights.push_back(f1);
	flights.push_back(f2);
	flights.push_back(f3);

	// terminate if convergence
	for (int iteration=0; iteration < maxIterations; iteration++) {

		std::sort(flights.begin(), flights.end());

		// break if points are close enough
		if (std::abs(flights[0].getTod() - flights[1].getTod()) < epsilon and
				std::abs(flights[0].getTof() - flights[1].getTof()) < epsilon and
				std::abs(flights[1].getTod() - flights[2].getTod()) < epsilon and
				std::abs(flights[1].getTof() - flights[2].getTof()) < epsilon
		)
		{
			break;
		}

		Flight centroid = (flights[0] + flights[1]) * 0.5;
		Flight worst = flights[2];
		Flight reflection = centroid + (centroid - worst) * r;
		calculateDeltaV(reflection);

		// if worse than best and better than second worst - replace worst and restart
		if (reflection >= flights[0] and reflection < flights[1]) {
			flights[2] = reflection;
			continue;
		}
		// if better than best - expandbool
		if (reflection < flights[0]) {

			Flight expansion = centroid + (centroid - worst) * e;
			calculateDeltaV(expansion);

			// if expansion better than reflection - replace
			if (expansion < reflection) {
				flights[2] = expansion;
				continue;
			} else {
				flights[2] = reflection;
				continue;
			}
		}

		// is worse than second worst
		Flight contraction = centroid + (centroid - worst) * c;
		calculateDeltaV(contraction);


		// if better than worst - replace
		if (contraction < flights[2]) {
			flights[2] = contraction;
			continue;
		}

		// reduction
		Flight best = flights[0];
		flights[1] = best + (flights[1] - best) * s;
		calculateDeltaV(flights[1]);
		flights[2] = best + (flights[2] - best) * s;
		calculateDeltaV(flights[2]);

	}

	Flight bestFlight = flights[0];

	return bestFlight;
}

void NelderMead::calculateDeltaV(Flight& flight) {
	if (
			flight.getTof() <= 0 or
			flight.getTof() > maximumTof or
			flight.getTod() < missionStart or
			flight.getTod() + flight.getTof() > missionStart + missionDuration
	)
	{
		flight.invalidate();
	} else {
		flight.getDeltaV(from, to);
	}
}


} /* namespace model */
