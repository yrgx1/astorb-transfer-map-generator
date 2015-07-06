/*
 * TransferBase.h
 *
 *  Created on: 23 Mar 2015
 *      Author: erik
 */

#ifndef TRANSFERBASE_H_
#define TRANSFERBASE_H_

#include <string>
#include <vector>
#include <deque>
#include <unordered_map>
#include <fstream>
#include <array>

#include "TransferWindow.h"

#define TRANSFERBASE_OUTPUT true


namespace model {
class Asteroid;

class Transferbase {
public:

	// tweak initial bucket counts of hash maps for performance gain, default ~10,
	// do not increase beyond lower bound of what is expected (causes excessive memory usage)
	static const size_t expected_number_of_origin_asteroids = 200000;
	static const size_t expected_number_of_destination_asteroids = 400;

	typedef unsigned int id_int;
	typedef std::unordered_map<Asteroid*, std::deque<TransferWindow>> TransferWindowIndex;

	Transferbase(const std::string& filename, const std::vector<Asteroid*> &asteroids);
	Transferbase(const std::string& filename, const std::vector<Asteroid*> &asteroids, double missionStart, double missionDuration, int nlderMeadDeltaV,
			int nelderMeadResolution, double nelderMeadEpsilon, int asteroidCount, double density);

	void create();
	void load(bool parameterCheck);
	static void finalizeTransferbase(Asteroid* startingPoint, std::vector<Asteroid*> asteroids, int spacecraftDeltaVlimit, const std::string &inputFilename, const std::string &outputFilename);
	bool fileExists();

	/**
	 * Vector elements must be grouped according to transfer window's target asteroid
	 */
	void write(Asteroid* from, const std::deque<TransferWindow> &transferWindowMap);
	std::deque<TransferWindow> getTransferWindows(Asteroid *from);
	bool lookup(const Asteroid* from) const;
	virtual ~Transferbase();

	void printTransferbase(Asteroid* start, short initialDeltaV, long printEveryN=250000000, size_t chunkSize=4096);

private:
	void removeNonOrigins();
	void removeJunkTime(Asteroid *startingPoint);
	void removeJunkCost(Asteroid *startingPoint, int spacecraftDeltaVlimit);
	void writeFromTransferbase(const TransferWindowIndex &transferWindowIndex);

	std::string filename;
	std::fstream filestream;

	int version = 1;
	double missionStart;
	double missionDuration;
	int nelderMeadDeltaV;
	int nelderMeadResolution;
	double nelderMeadEpsilon;
	int asteroidCount;
	double density;

	std::unordered_map<Asteroid*, id_int> asteroidIndex;
	std::unordered_map<id_int, Asteroid*> idIndex;
//	TransferIndex transferIndex;
//	std::deque<Transfer> transfers;
	TransferWindowIndex transferWindowIndex;

	void writeDouble(double value) {
		filestream.write(reinterpret_cast<char*>(&value), sizeof(value));
	}
	void writeInt(int value) {
			filestream.write(reinterpret_cast<char*>(&value), sizeof(value));
	}
	void writeChar(char value) {
		filestream.write(&value, sizeof(char));
	}
	void writeShort(short value) {
		filestream.write(reinterpret_cast<char*>(&value), sizeof(value));
	}
	void writeString(const std::string& value) {
		filestream.write(value.c_str(),sizeof(char)*value.length());
	}

	double readDouble() {
		char bytes[sizeof(double)/sizeof(char)];
		filestream.read(bytes, sizeof(double));
		double* value = reinterpret_cast<double*>(bytes);
		return *value;
	}

	int readInt() {
		char bytes[sizeof(int)/sizeof(char)];
		filestream.read(bytes, sizeof(int));
		int* value = reinterpret_cast<int*>(bytes);
		return *value;
	}

	short readShort() {
		char bytes[sizeof(int)/sizeof(short)];
		filestream.read(bytes, sizeof(short));
		short* value = reinterpret_cast<short*>(bytes);
		return *value;
	}

	char readChar() {
		char value;
		filestream.read(&value, sizeof(char));
		return value;
	}
	std::string readName() {
		char bytes[26];
		filestream.read(bytes, sizeof(char) * 26);
		return std::string(bytes, 26);
	}
};

} /* namespace model */

#endif /* TRANSFERBASE_H_ */
