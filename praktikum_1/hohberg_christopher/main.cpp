/*
 * main.cpp
 *
 *  Created on: Oct 24, 2021
 *      Author: debian
 */

#include <iostream>
#include <sys/time.h>
#include <sys/resource.h>
using namespace std;


int arraySize = 400000000;
int recCounter = 0;


void funcRec(int* start) {
	int end = 0;
	if (recCounter % 10000 == 0) {
		int result = 0;
		result = &end - start;
		cout << "current stack size: " << result << endl;
		cout << "start " << start << endl;
		cout << "end " << &end << endl;
		cout << recCounter << endl;
	}
	recCounter++;
	funcRec(start);
}

void funcMem() {
	cout << "called funcMem" << endl;

	struct rlimit stackLimit, speicherLimit;
	struct rusage start,end;
	getrusage(RUSAGE_SELF, &start);

	int *numbers = new int[arraySize];

	for (int i = 0; i < arraySize; i++) {
		numbers[i] = i;
		if (i % 100000 == 0) {
			getrusage(RUSAGE_SELF, &end);
			cout << "system time: " << end.ru_stime.tv_usec - start.ru_stime.tv_usec << " usec" << endl;
			cout << "user time: " << end.ru_utime.tv_usec - start.ru_utime.tv_usec << " usec" << endl;
			cout << "verwendeter speicher " << end.ru_maxrss << endl;
			getrlimit(RLIMIT_STACK, &stackLimit);
			cout << "stack limit soft: " << stackLimit.rlim_cur << endl;
			cout << "stack limit max: " << stackLimit.rlim_max << endl;
			getrlimit(RLIMIT_RSS, &speicherLimit);
			cout << "speicher max limit: " << speicherLimit.rlim_max << endl;
			cout << "speicher min limit: " << speicherLimit.rlim_cur << endl << endl;
		}
	}
	delete []numbers;
}


int main() {
	funcMem();
	int start = 0;
	funcRec(&start);
	return 0;
}


