#include <iostream>
#include "secret.h"
#include "interactiveModule.h"
#include "headlessModule.h"

using namespace std;

int main(int argc, char **argv) {
	if(argc == 1) {
		InteractiveModule m;
		return m.run();
	} else {
		HeadlessModule m(argc, argv);
		return m.run();
	}
}

