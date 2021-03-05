#ifndef HEADLESS_MODULE
#define HEADLESS_MODULE
#include "baseModule.h"

class HeadlessModule: public BaseModule {
public:
	enum Query {
		READ,
		WRITE,
		DELETE,
		INVAL
	};

private:
	int argc;
	char **argv;
	bool defaultFile;
	bool json;
	HeadlessModule::Query q;

	std::string key;
	std::string dec;
	std::string tag;
	int idx;

	void read_secrets();
	void write_secrets();
	void delete_secrets();

public:
	HeadlessModule(int argc, char **argv);
	int run();
};

#endif
