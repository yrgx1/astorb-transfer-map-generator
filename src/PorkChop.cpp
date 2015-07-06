/*
 * PorkChop.cpp
 *
 *  Created on: 14 Mar 2015
 *      Author: erik
 */

#include "PorkChop.h"

#include "Lambert.h"
#include <fstream>
#include <vector>
#include <sstream>
#include <inttypes.h>

namespace model {


using std::vector;

void PorkChop::generateTof(std::string filename) {

	vector<vector<double>> graph;
	graph.resize(dimx);
	for (int i=0; i < dimx; i++) {
		graph[i].resize(dimy);
	}

	double start = missionStart.mjd2000(), duration = (missionEnd.mjd2000() - missionStart.mjd2000());
	double stepX = duration / static_cast<double>(dimx);
	double stepY = maxTof / static_cast<double>(dimy+1);

	double minimum = 0;
	double maximum = maxDeltaV;

	for (int x=0; x<dimx; x++) {
		double tod = start + stepX * x;
		for (int y=0; y<dimy; y++) {
			double tof = stepY * (y+1);

			double deltaV = lambert::calculateDeltaV(&asteroid1, &asteroid2, tod, tof);

//			if (deltaV > maximum) {
//				maximum = deltaV;
//			}
			if (deltaV > maximum) {
				deltaV = maximum;
			}

			graph[y][x] = deltaV;

		}
	}



	// every point is normalized to the interval [minimum,maximum]
	// normalizedPoint = static_cast<int>((point - minimum) / (maximum - minimum) * 256)

	double factor = 255.0 / (maximum - minimum);

	vector<vector<uint8_t>> normalizedGraph;
	normalizedGraph.resize(dimx);
	for (int i=0; i < dimx; i++) {
		normalizedGraph[i].resize(dimy,0);
	}
	if (minimum < maximum) {
		for (int x=0; x<dimx; x++) {
			for (int y=0; y<dimy; y++) {
				normalizedGraph[x][y] = static_cast<uint8_t>(255) - static_cast<uint8_t>((graph[x][y] - minimum) * factor);
			}
		}
	}


	// print to file

	std::stringstream builder;
	builder << filename << "_" << dimx << "x" << dimy << ".binary";
	std::string appendedFilename = builder.str();

	std::ofstream filestream(appendedFilename, std::ios::binary);

	for (int x=0; x<dimx; x++) {
		for (int y=0; y<dimy; y++) {
			filestream << normalizedGraph[x][y];
		}
	}

	filestream.close();
}

void PorkChop::generateArrival(std::string filename, epoch maximumTod, epoch minimumToa) {
	double minimum = 1E200;
	double maximum = maxDeltaV;


	vector<vector<double>> graph;
	graph.resize(dimx);
	for (int i=0; i < dimx; i++) {
		graph[i].resize(dimy,maximum);
	}

	double start = missionStart.mjd2000();
	double todRange = maximumTod.mjd2000() - missionStart.mjd2000();
	double toaRange = missionEnd.mjd2000() - minimumToa.mjd2000();

	double stepX = todRange / static_cast<double>(dimx);
	double stepY = toaRange / static_cast<double>(dimy);


	for (int x=0; x<dimx; x++) {
		double tod = start + stepX * x;
		for (int y=0; y<dimy; y++) {
			double arrival = minimumToa.mjd2000() + stepY * y;
			double tof = arrival - tod;


			double deltaV = lambert::calculateDeltaV(&asteroid1, &asteroid2, tod, tof);

			if (deltaV < minimum) {
				minimum = deltaV;
			}
			if (deltaV > maximum) {
				deltaV = maximum;
			}

			graph[y][x] = deltaV;

		}
	}

	// every point is normalized to the interval [minimum,maximum]
	// normalizedPoint = static_cast<int>((point - minimum) / (maximum - minimum) * 256)

	double factor = 255.0 / (maximum - minimum);

	vector<vector<uint8_t>> normalizedGraph;
	normalizedGraph.resize(dimx);
	for (int i=0; i < dimx; i++) {
		normalizedGraph[i].resize(dimy,0);
	}
	if (minimum < maximum) {
		for (int x=0; x<dimx; x++) {
			for (int y=0; y<dimy; y++) {
				normalizedGraph[x][y] = static_cast<uint8_t>(255) - static_cast<uint8_t>((graph[x][y] - minimum) * factor);
			}
		}
	}


	// print to file

	std::stringstream builder;
	builder << filename << "_" << dimx << "x" << dimy << ".binary";
	std::string appendedFilename = builder.str();

	std::ofstream filestream(appendedFilename, std::ios::binary);

	for (int x=0; x<dimx; x++) {
		for (int y=0; y<dimy; y++) {
			filestream << normalizedGraph[x][y];
		}
	}

	filestream.close();
}

PorkChop::~PorkChop() {
	// TODO Auto-generated destructor stub
}

} /* namespace model */
