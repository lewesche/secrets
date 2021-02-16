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
    cout << "  r \t read all secrets" << endl;
    cout << "  w \t write new secret" << endl;
    cout << "  f \t find and read secret(s) by tag or index" << endl << "    \t   indicies are integers, tags are not" << endl;
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
    cout << "  enter tag/index: ";
    getline(cin, target_tag);
    if(is_int(target_tag)) {
        target_idx = stoi(target_tag);
        target_tag = "";
    } else {
        target_idx = -1;
    }
}


// Reads secret based on tag or idx
// Reads all secrets if target_idx = -1 and target_tag is empty
void InteractiveModule::read_secrets(const string& target_tag, const int target_idx) {
    cout << "  enter a key: ";
    string key;
    getline(cin, key);

    cout << "  " << "idx" << '\t' << "tag" << endl;
    cout << "    " << '\t' << "secret" << endl;

	vector<secret*> res = base_read(key, target_tag, target_idx);
	printSecrets(res);
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

	base_write(key, dec, tag);
}

int InteractiveModule::run() {
    while(1) {
        string target_tag; // c++ didn't like this in the switch statement
        int target_idx=-1;

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
                read_secrets("", -1);
                break;
            case 'w':
                write_secrets();
                break;
            case 'f':
                get_targets(target_tag, target_idx);
                read_secrets(target_tag, target_idx);
                break;
            case 'l':
                list_secrets();
                break;
            case 'd':
                get_targets(target_tag, target_idx);
                delete_secrets(target_tag, target_idx);
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

