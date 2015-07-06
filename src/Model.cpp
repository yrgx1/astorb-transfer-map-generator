//============================================================================
// Name        : Model.cpp
// Author      : Erik Samuelsson
// Version     :
// Copyright   : GPLv3
// Description : Hello World in C, Ansi-style
//============================================================================

#include "../pykep/src/keplerian_toolbox.h"
#include "NelderMead.h"
#include "Flight.h"
#include "PorkChop.h"
#include "AstorbReader.h"
#include "Asteroid.h"
#include "utility"
#include "FilterAsteroids.h"
#include "Transferbase.h"

#include <iostream>
#include <boost/program_options.hpp>

namespace po = boost::program_options;
using std::cout;
using std::endl;

const std::string sourceDatabase = "astorb.dat_26_02_15";
const std::string filteredDatabase = "astorb.filtered_asteroid";
const std::string progressFile = "transferbase.bin";

kep_toolbox::epoch start(365.25 * (2020-2000));
double duration(365.25*20);
double maxTof(365.25*4);
int nelderMeadGridResolution = 15; // 7
int epsilon = 1;
int maxDeltaV = 3424;
int maxDatabaseDeltaV = 1700;
int nelderMeadDeltaV = 1200;
int porkchopResolution = 1000;
double asteroidsDensity = 1.0;


bool print = false;
bool filterAsteroids = false;
bool search = false;
bool noResume = false;
std::string printBranching = "";
std::string porkchop = "";
std::string finalize = "";


model::Asteroid hive("       HIVE               ", "     ", 2.6, 0, 0, 0, 0, 0);

void printValues() {
	cout << "Start epoch: " << start.mjd2000() << " (" << start << ")" <<endl;
	cout << "Duration: " << duration << endl;
	cout << "Asteroid density: " << asteroidsDensity << endl;
	cout << "BEE delta v limit: " << maxDeltaV << endl;
	cout << "Filter delta v limit: " << maxDatabaseDeltaV << endl;
	cout << "Nelder Mead delta v limit (max transfer cost) : " << nelderMeadDeltaV << endl;
	cout << "Nelder Mead resolution: " << nelderMeadGridResolution << endl;
	cout << "Nelder Mead epsilon: " << epsilon << endl;
	cout << "Porkchop resolution: " << porkchopResolution << endl;
	cout << endl;
}

po::variables_map parseCommandLine(int argc, char** argv) {
	po::options_description desc("Allowed options");
	desc.add_options()
		("help", "produce help message")
		("print", po::bool_switch(&print), "print settings")

		("beedv", po::value<int>(&maxDeltaV), "set BEE delta v limit when searching")
		("transferdv", po::value<int>(&nelderMeadDeltaV), "set maximum allowed transfer cost between asteroids when searching")
		("filterdv", po::value<int>(&maxDatabaseDeltaV), "set maximum allowed delta v distance from HIVE when filtering")
		("porkchop-resolution", po::value<int>(&porkchopResolution), "set resolution of generated porkchop")
		("density", po::value<double>(&asteroidsDensity), "set percentage ([0,1]) of asteroids in database to include")
		("maxTof", po::value<double>(&maxTof), "set maximum tof in days")
		("duration", po::value<double>(&duration), "set mission duration in days")


		("filter", po::bool_switch(&filterAsteroids), "filter source database")
		("search", po::bool_switch(&search), "perform search in filtered database")
		("print-branching", po::value<std::string>(&printBranching), "Print number of routes and asteroids at each degrees of separation from the initial point in the search in file.")
		("finalize", po::value<std::string>(&finalize), "\"oldfilename,newfilename\", strips away non-reachable asteroids and unusable transfer windows, requires correct beedv.")
		("no-resume", po::bool_switch(&noResume), "do not resume from previous progress. Overwrites progress file!!")
		("porkchop", po::value<std::string>(&porkchop), "\"[from],[to]\". Generates a binary of porkchop from [from] to [to]")
	;


	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	if (vm.count("help")) {
		cout << desc << endl;
	}

	return vm;
}

int main(int argc, char** argv) {

	double before, after;

	auto vm = parseCommandLine(argc, argv);

	if (vm.count("help")) {
		return 1;
	}

	printValues();

	if (print) {
		return 0;
	}

	model::NelderMead::setEpsilon(epsilon);
	model::NelderMead::setMaxIterations(100);
	model::NelderMead::setMissionDuration(duration);
	model::NelderMead::setMaximumTof(maxTof);
	model::NelderMead::setMissionStart(start);
	model::NelderMead::setResolution(nelderMeadGridResolution);
	model::NelderMead::setMaximumDeltaV(nelderMeadDeltaV);

	// generate filtered database
	if (filterAsteroids) {
		model::NelderMead::setMaximumDeltaV(maxDatabaseDeltaV);
		model::FilterAsteroids().run(&hive, sourceDatabase, filteredDatabase, true);
		model::NelderMead::setMaximumDeltaV(nelderMeadDeltaV);
	}

	// perform search
	if (search) {

		// load filtered database
		std::vector<model::Asteroid*> asteroids;
		before = model::getMillis();
		asteroids = model::AstorbReader(filteredDatabase).readAsteroids(asteroidsDensity);
		asteroids.push_back(&hive);
		after = model::getMillis();

		cout << "Database contains " << asteroids.size() << " asteroids, loaded in " << after - before << " ms." << endl;
		cout << endl;

		model::Transferbase transferbase(progressFile, asteroids, start.mjd2000(), duration, nelderMeadDeltaV, nelderMeadGridResolution, epsilon, static_cast<int>(asteroids.size()), asteroidsDensity);

		if (transferbase.fileExists() and not noResume) {
			// resume previous progress
			before = model::getMillis();
			transferbase.load(true);
			after = model::getMillis();
			if (not noResume) {
				cout << "Resumed from previous progress in " << after - before << " ms." << endl << endl;
			}
		} else {
			// create new database
			transferbase.create();
		}

		before = model::getMillis();
		hive.calculateTransfers(asteroids, &transferbase, maxDeltaV, static_cast<int>(start.mjd2000()));
		after = model::getMillis();
		cout << "Performed search in " << (after - before) / 1000.0 << " s." << endl << endl;

	}


	if (finalize.length() > 0) {
		// split filenames
		size_t commaIndex = finalize.find(',');

		if (commaIndex == std::string::npos) {
			cout << "Could not parse the filenames" << endl;
			return 1;
		}

		std::string inputFile = finalize.substr(0,commaIndex), outputFile = finalize.substr(commaIndex+1, finalize.length() - commaIndex - 1);

		// load unfiltered database
		std::vector<model::Asteroid*> asteroids;
		before = model::getMillis();
		asteroids = model::AstorbReader(filteredDatabase).readAsteroids(1);
		asteroids.push_back(&hive);
		after = model::getMillis();

		before = model::getMillis();
		model::Transferbase::finalizeTransferbase(&hive, asteroids, maxDeltaV, inputFile, outputFile);
		after = model::getMillis();

		cout << "Finalized transfer database in " << after - before << "ms." << endl;
	}

	if (porkchop.length() > 0) {

		// load unfiltered database
		std::vector<model::Asteroid*> asteroids;
		before = model::getMillis();
		asteroids = model::AstorbReader(sourceDatabase).readAsteroids(1);
		asteroids.push_back(&hive);
		after = model::getMillis();

		cout << "Database contains " << asteroids.size() << " asteroids, loaded in " << after - before << " ms." << endl;
		cout << endl;

		asteroids.push_back(&hive);

		// find asteroid names
		size_t commaIndex = porkchop.find(',');

		if (commaIndex == std::string::npos) {
			cout << "Could not parse the asteroid names" << endl;
			return 1;
		}

		std::string fromString = porkchop.substr(0,commaIndex), toString = porkchop.substr(commaIndex+1, porkchop.length() - commaIndex - 1);

		cout << "Generating porkchop from " << fromString << " to " << toString << endl;

		// find asteroids
		model::Asteroid *from=nullptr, *to=nullptr;
		for (model::Asteroid* asteroid : asteroids) {
			if (asteroid->getName().find(fromString) != std::string::npos) {
				from = asteroid;
			}
			if (asteroid->getName().find(toString) != std::string::npos) {
				to = asteroid;
			}
		}

		if (from == nullptr) {
			cout << "could not find an asteroid in database named " << fromString << endl;
			return 1;
		} else {
			cout << "Found from asteroid:\t" << from->getName() << " " << from->getPlanet().get_elements() << endl;
		}
		if (to == nullptr) {
			cout << "could not find an asteroid in database named " << toString << endl;
			return 1;
		} else {
			cout << "Found to asteroid:\t" << to->getName()<< " " << to->getPlanet().get_elements() << endl;
		}

		bool porkchopTof = true;
		cout << "Generating pork-chop plot" << endl;
		model::PorkChop porkchop(from->getPlanet(), to->getPlanet(), porkchopResolution, porkchopResolution, start, kep_toolbox::epoch(start.mjd2000() + duration), -1, maxDeltaV);
//		model::PorkChop porkchop(kep_toolbox::planet_ss("Earth"), kep_toolbox::planet_ss("Mars"), porkchopResolution, porkchopResolution, start, kep_toolbox::epoch(start.mjd2000() + duration), -1, maxDeltaV);
		if (porkchopTof) {
			porkchop.generateTof("porkchop");
		} else {
			porkchop.generateArrival("porkchop", kep_toolbox::epoch(2137), kep_toolbox::epoch(2161));
		}
	}

	if (printBranching.length() > 0) {
		cout << "Printing branching for file \"" << printBranching << "\""  << endl;
		// load filtered database
		std::vector<model::Asteroid*> asteroids;
		before = model::getMillis();
		asteroids = model::AstorbReader(filteredDatabase).readAsteroids(asteroidsDensity);
		asteroids.push_back(&hive);
		after = model::getMillis();

		model::Transferbase transferbase(printBranching, asteroids);
		transferbase.load(false);
		transferbase.printTransferbase(&hive, maxDeltaV);
	}


#if NELDER_MEAD_DEBUG
	model::NelderMead::printDebug();
#endif

	cout << "finished" << endl;

}
