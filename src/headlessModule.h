#ifndef HEADLESS_MODULE
#define HEADLESS_MODULE
#include "baseModule.h"

class HeadlessModule: public BaseModule {
public:
	enum Query {
		READ,
		WRITE,
		FIND,
		LIST,
		DELETE,
		INVAL
	};

private:
	void read_secrets(const std::string& target_tag, const int target_idx);
	void write_secrets();
	int argc;
	char **argv;
	bool defaultFile;
	bool json;
	HeadlessModule::Query q;

	std::string key;
	std::string dec;
	std::string tag;
	int idx;

public:
	HeadlessModule(int argc, char **argv);
	int run();
};

#endif
