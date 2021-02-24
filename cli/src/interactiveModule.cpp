#include <iostream>
#include <string>
#include <fstream>
#include "secret.h"
#include "interactiveModule.h"
#include "baseModule.h"

using namespace std;

InteractiveModule::InteractiveModule() {
	print_intro();
    string homepath = getenv("HOME");
    fname = string(homepath + "/.secrets.txt");
    test_path();
}

void InteractiveModule::print_help() {
    cout << "  h \t show all commands" << endl;
    cout << "  r \t read all secret(s) by tag or numerical index" << endl;
    cout << "  w \t write new secret" << endl;
    cout << "  l \t list all tags/indicies" << endl;
    cout << "  d \t delete secret(s) by tag or index" << endl << "    \t   does nothing if no matches are found" << endl;
    cout << "  p \t use new path to secrets file" << endl << "    \t   creates a new file if the file is not found" << endl;
    cout << "  q \t quit" << endl << endl;
}

void InteractiveModule::print_intro() {
    cout << "    ~~~~~ secrets ~~~~~" << endl;
    cout << "  'h' to see all commands" <<  endl << endl;
}

bool InteractiveModule::is_int(string str) {
    if(str.empty())
        return false;
    for(size_t i=0; i<str.length(); i++) {
        if(!isdigit(str[i]))
            return false;
    }
    return true;
}

void InteractiveModule::get_targets(string& target_tag, int& target_idx) {
    cout << "  enter tag (optional): ";
    getline(cin, target_tag);

    cout << "  enter index (optional): ";
	string buf;
    getline(cin, buf);

	if(!buf.empty() && is_int(buf)) {
        target_idx = stoi(buf);
    } else {
        target_idx = -1;
    }
}

// Reads secret based on tag or idx
// Reads all secrets if target_idx = -1 and target_tag is empty
void InteractiveModule::read_secrets() {
	cout << "  enter a key: ";
    string key;
    getline(cin, key);

	string tag; 
	int idx;
	get_targets(tag, idx);

    cout << "  " << "idx" << '\t' << "tag" << endl;
    cout << "    " << '\t' << "secret" << endl;

	vector<secret*> res = base_read(key, tag, idx);
	print_secrets(res);
}

void InteractiveModule::write_secrets() {
    cout << "  enter a key: ";
    string key;
    getline(cin, key);

    cout << "  enter phrase to encrypt: ";
    string dec;
    getline(cin, dec);

    cout << "  optionally, enter a non-integer tag: ";
    string tag;
    getline(cin, tag);

	int status = base_write(key, dec, tag);
	if(status == 0) {
		print_success();
	} else {
		print_failure();
	}
}

void InteractiveModule::list_secrets() {
	string tag; 
	int idx;
	get_targets(tag, idx);

    cout << "  " << "idx" << '\t' << "tag" << endl;

	vector<secret*> res = base_list(tag, idx);	
	print_secrets(res);
}

void InteractiveModule::delete_secrets() {
	string tag; 
	int idx;
	get_targets(tag, idx);

	int status = base_delete(tag, idx);	
	if(status == 0) {
		print_success();
	} else {
		print_failure();
	}
}

int InteractiveModule::run() {
    while(1) {
        cout << "  enter command (r/w/f/l/d/p/q/h): ";
        char select;
        cin >> select;
        string trash;
        getline(cin, trash);

        // In case user entered more than 1 char
        cin.clear();
        fflush(stdin);

        switch(select) {
            case 'h':
                print_help();
                break;
            case 'r':
                read_secrets();
                break;
            case 'w':
                write_secrets();
                break;
            case 'l':
                list_secrets();
                break;
            case 'd':
                delete_secrets();
                break;
            case 'p':
                cout << "  enter filename/path: ";
                getline(cin, fname);
                test_path();
                break;
            case 'q':
                return 0;
        }
    }

}

