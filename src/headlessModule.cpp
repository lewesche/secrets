#include <iostream>
#include <string>
#include <fstream>
#include "secret.h"
#include "headlessModule.h"
#include "baseModule.h"

using namespace std;

HeadlessModule::HeadlessModule(int c, char **v) : argc{c}, argv{v} {
	key = "";
	dec = "";
	tag = "";
	idx = -1;
	q = INVAL;
	fname = "";
	defaultFile = true;
	json = false;
}

void HeadlessModule::read_secrets(const string& target_tag, const int target_idx) {
		vector<secret*> res = base_read(key, tag, idx);
		if(json) {
			printJson(res);
		} else {
			printSecrets(res);
		}
}

void HeadlessModule::write_secrets() {
		base_write(key, dec, tag);
}

int HeadlessModule::run() {
	for(int i=1; i<argc; ++i) { 
		if(argv[i][0] == '-') {
			char c = argv[i][1];
			switch(c) {
            case 'r':
				if(q != INVAL) {
					return -1;
				}
				q = READ;
                break;
            case 'w':
				if(q != INVAL) {
					return -1;
				}
				q = WRITE;
				dec = argv[i+1];
				++i;
                break;
            case 'f':
				if(q != INVAL) {
					return -1;
				}
				q = FIND;
                break;
            case 'l':
				if(q != INVAL) {
					return -1;
				}
				q = LIST;
                break;
            case 'd':
				if(q != INVAL) {
					return -1;
				}
				q = DELETE;
                break;
			case 'k':
				key = argv[i+1];
				++i;
				break;
			case 'i':
				idx = atoi(argv[i+1]);
				++i;
				break;
			case 't':
				tag = argv[i+1];
				++i;
				break;
			case 'j':
				json = true;
				break;
            case 'p':
				defaultFile = false;
				fname = argv[i+1];
				++i;
                break;
			default:
				return -1;
       		} 
		}
	}
	if(defaultFile) {
		string homepath = getenv("HOME");
    	fname = string(homepath + "/.secrets.txt");
    	test_path();
	}

	switch(q) {
	case READ:
		read_secrets("", -1);
		break;
	case WRITE:
		write_secrets();
		break;
	case FIND:
		read_secrets(tag, idx);
		break;
	case LIST:
		list_secrets();
		break;
	case DELETE:
		delete_secrets(tag, idx);
		break;
	case INVAL:
		return -1;
	}

	return 0;
}

