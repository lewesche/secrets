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

void HeadlessModule::read_secrets() {
		vector<secret*> res = base_read(key, tag, idx);
		if(json) {
			print_secrets_json(res);
		} else {
			print_secrets(res);
		}
}

void HeadlessModule::write_secrets() {
		int status = base_write(key, dec, tag);
		if(status == 0) {
			if(json) {
				print_success_json();
			} else {
				print_success();
			}
		} else {
			if(json) {
				print_failure_json();
			} else {
				print_failure();
			}
		}
}

void HeadlessModule::delete_secrets() {
		int status = base_delete(tag, idx);
		if(status == 0) {
			if(json) {
				print_success_json();
			} else {
				print_success();
			}
		} else {
			if(json) {
				print_failure_json();
			} else {
				print_failure();
			}
		}
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
				while(i+1 < argc && argv[i+1][0] != '-') {
					dec += ' ';
					dec += argv[i+1];
					++i;
				}
                break;
            case 'd':
				if(q != INVAL) {
					return -1;
				}
				q = DELETE;
                break;
			case 'k':
				if(i+1 >= argc || argv[i+1][0]=='-') {
					//reading/writing with a blank key
					break;
				}
				key = argv[i+1];
				++i;
				while(i+1 < argc && argv[i+1][0] != '-') {
					key += ' ';
					key += argv[i+1];
					++i;
				}
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
				test_path();
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
		read_secrets();
		break;
	case WRITE:
		write_secrets();
		break;
	case DELETE:
		delete_secrets();
		break;
	case INVAL:
		return -1;
	}

	return 0;
}

