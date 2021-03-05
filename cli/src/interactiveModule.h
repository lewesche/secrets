#ifndef INTERACTIVE_MODULE
#define INTERACTIVE_MODULE
#include "baseModule.h"

class InteractiveModule: public BaseModule {
private:
	void print_help(); 
	void print_intro();
	bool is_int(std::string str);
	void get_targets(std::string& target_tag, int& target_idx);
	
	void read_secrets();
	void write_secrets();
	void delete_secrets();
public:
	InteractiveModule();
	int run();
};

#endif
