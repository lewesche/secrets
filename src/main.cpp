#include <iostream>
#include <string>
#include <fstream>
#include "secret.h"

using namespace std;

// Reads secret based on tag or idx
// Reads all secrets if target_idx = -1 and target_tag is empty
void read_secrets(const string& fname, const string& target_tag, const int target_idx) {
	cout << "  enter a key: "; 
	string key;
	getline(cin, key); 

	ifstream file(fname);
	if(!file) {
		cerr << "  Could not open file" << endl;
		return;
	}

	cout << "  " << "idx" << '\t' << "tag" << endl;
	cout << "    " << '\t' << "seceret" << endl;

	size_t i=0;
	while(!file.eof()) {
		string tag;
		getline(file, tag);
		string enc;
		getline(file, enc);
		if(!enc.empty()) {
			//enc = numstr_2_charstr(enc);
			if((target_tag.empty() && target_idx==-1) || (!target_tag.empty() && target_tag==tag) || target_idx==i) {
				//string dec = decrypt(enc, key);
				secret s;
				s.set_key(key);
				s.set_enc(enc);
				s.decrypt();
				cout << "  " << i << '\t' << tag << endl;
				cout << "    " << '\t' << s.get_dec() << endl;
			}
			++i;
		}
	}
	cout << endl;
	file.close();
}

void write_secrets(const string& fname) {
	ofstream file(fname, std::ios::app);
	if(!file) {
		cerr << "Could not open file" << endl;
		return;
	}

	cout << "  enter a key: "; 
	string key; 
	getline(cin, key); 

	cout << "  enter phrase to encrypt: "; 
	string dec; 
	getline(cin, dec); 

	cout << "  optionally, enter a non-integer tag: "; 
	string tag; 
	getline(cin, tag); 

	if(dec.empty()) {
		cerr << "Can't write - phrase is empty" << endl << endl;
	} else {
		secret s;
		s.set_key(key);
		s.set_dec(dec);
		s.encrypt();

		file << tag << endl;
		s.write(file);
		cout << "  Done" << endl << endl;
	}
	file.close();
}

void list_secrets(const string& fname) {
	ifstream file(fname);
	if(!file) {
		cerr << "  Could not open file" << endl << endl;
		return;
	}

	cout << "  " << "idx" << '\t' << "tag" << endl;

	size_t i=0;
	while(!file.eof()) {
		string tag;
		getline(file, tag);
		string enc;
		getline(file, enc); // not used
		if(!enc.empty() && !tag.empty()) {
			cout << "  " << i << '\t' << tag << endl;
			++i;
		}
	}
	cout << endl;
	file.close();

}

void delete_secrets(const string& fname, const string& target_tag, const int target_idx) {
	ifstream file(fname);
	if(!file) {
		cerr << "  Could not open file" << endl << endl;
		return;
	}

	string fname_temp = fname + ".tmp";
	ofstream temp;
	temp.open(fname_temp, ofstream::out);

	size_t i=0;
	while(!file.eof()) {
		string tag;
		getline(file, tag);
		string enc;
		getline(file, enc);
		if(!enc.empty()) {
			if((!target_tag.empty() && target_tag==tag) || target_idx==i) {
				++i;
				continue;
			}
			temp << tag << endl;
			temp << enc << endl;
			++i;
		}
	}

	file.close();
	temp.close();
	remove(fname.c_str());
	rename(fname_temp.c_str(), fname.c_str());
	cout << "  Done" << endl << endl;
}

void print_help() {
	cout << "  h \t show all commands" << endl;
	cout << "  r \t read all secrets" << endl;
	cout << "  w \t write new secret" << endl;
	cout << "  f \t find and read secret(s) by tag or index" << endl << "    \t   indicies are integers, tags are not" << endl;
	cout << "  l \t list all tags/indicies" << endl;
	cout << "  d \t delete secret(s) by tag or index" << endl << "    \t   does nothing if no matches are found" << endl;
	cout << "  p \t use new path to secrets file" << endl << "    \t   creates a new file if the file is not found" << endl;
	cout << "  q \t quit" << endl << endl;
}

void print_intro() {
	cout << "    ~~~~~ secrets ~~~~~" << endl;
	cout << "  'h' to see all commands" <<  endl << endl;
}

void test_path(string fname) {
	ifstream ifile(fname);
	if(!ifile){
		cout << "  Couldn't find " << fname << ", creating file." << endl << endl;
		ofstream ofile(fname);
	} else {
		ifile.close();
	}
}

bool is_int(string str) {
	if(str.empty())
		return false;
	for(size_t i=0; i<str.length(); i++) {
		if(!isdigit(str[i]))
			return false;
	}
	return true;
}

void get_targets(string& target_tag, int& target_idx) {
	cout << "  enter tag/index: ";
	getline(cin, target_tag);
	if(is_int(target_tag)) {
		target_idx = stoi(target_tag);
		target_tag = "";
	} else {
		target_idx = -1;
	}
}

int main(int argc, char **argv) {
	if(argc > 1)
		if(!strcmp(argv[1], "test"))
			return 0;

	print_intro();
	string homepath = getenv("HOME");
	string fname(homepath + "/.secrets.txt");
	test_path(fname);

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
				read_secrets(fname, "", -1);
				break;
			case 'w':
				write_secrets(fname);
				break;
			case 'f':
				get_targets(target_tag, target_idx);
				read_secrets(fname, target_tag, target_idx);
				break;
			case 'l':
				list_secrets(fname);
				break;
			case 'd':
				get_targets(target_tag, target_idx);
				delete_secrets(fname, target_tag, target_idx);
				break;
			case 'p':
				cout << "  enter filename/path: ";
				getline(cin, fname);
				test_path(fname);
				break;
			case 'q':
				return 0;
		}
	}
}

