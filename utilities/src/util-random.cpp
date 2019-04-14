#include "stdafx.h"

#include <random>


void printRandomValues() {
	using namespace std;

	default_random_engine rndGen;
	double mu = 5;
	double sig = 2;
	normal_distribution<double> normDist(mu,sig);
	const int iExperiments = 1000;
	int probability[11] = {};
	for (int i = 0; i < iExperiments; ++i) {
		double number = normDist(rndGen);
		if (number >= 0 && number <= 10) 
			++probability[static_cast<int>(number)];
	}

	cout << "histogram normal deviation (mu = " << mu << ", sig =" << sig << ")" << endl;
	for (int i = 0; i <= 10; ++i) {
		cout << setw(2) << i << ": ";
		cout << setw(4) << probability[i] << " ";
		cout << string(probability[i] / 10, '*') << endl;
	}

	return;
}