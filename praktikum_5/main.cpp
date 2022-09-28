#include <iostream>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
using namespace std;


int arraySize = 600000000;
int *numbers = new int[arraySize];


void funcMem() {
    cout << "start funcmem" << endl;
	for (int i = 0; i < arraySize; i++) {
		numbers[i] = i;
		}
    cout << "end funcmem" << endl;
}

int main() {
	funcMem();
    cout << "entering while loop" << endl;
    while(true) {
        cout << "start for loop" << endl;
        for(int i = 0; i < 1000000000; i++) {
        }
        cout << "start sleeping" << endl;
        usleep(1000000);
    }
    delete []numbers;
	return 0;
}