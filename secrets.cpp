#include <iostream>
#include <string>
#include <fstream>

using namespace std;

string op(const string& pass, const string& master, int multiplier) {
	size_t i=0;
	size_t j=0;
	bool seen_full_pass = false;
	bool seen_full_master = false;
	string res = pass;

	while(!seen_full_pass || !seen_full_master) {
		if(j>=master.size()) {
			j=0;
			seen_full_master = true;
		}
		if(i>=pass.size()) {
			i=0;
			seen_full_pass = true;
		}

		res[i] = res[i] + master[j]*multiplier; 
		++j;
		++i;
	}

	return res;
}

string encrypt(const string& pass, const string& master) {
	return op(pass, master, 5*master.size());
}

string decrypt(const string& pass, const string& master) {
	return op(pass, master, -5*master.size());
}

string numstr_2_charstr(const string& enc) {
	string res;
	for(size_t i=0; i<enc.length(); i+=3) {
		string substr;
		substr += enc[i];
		substr += enc[i+1];
		substr += enc[i+2];
		char c = stoi(substr);	
		res += c;
	}
	return res;
}

void read_secrets(const string& fname, const string& target_tag, const int target_idx) {
	cout << "  enter a key: "; 
	string master;
	getline(cin, master); 

	ifstream file(fname);
	if(!file) {
		cerr << "  Could not open file" << endl;
	}

	size_t i=0;
	while(!file.eof()) {
		string tag;
		getline(file, tag);
		string enc;
		getline(file, enc);
		if(!enc.empty()) {
			enc = numstr_2_charstr(enc);
			if((target_tag.empty() && target_idx==-1) || (!target_tag.empty() && target_tag==tag) || target_idx==i) {
				string dec = decrypt(enc, master);
				cout << "  " << i << '\t' << tag << endl;
				cout << "   " << '\t' << dec << endl;
			}
			++i;
		}
	}
	cout << endl;
	file.close();
}

void write_as_nums(ofstream& file, const string& enc) {
	for(size_t i=0; i<enc.length(); ++i) {
		unsigned char c = enc[i];
		if(c<10) {
			file << "00" << +c;
		} else if(c<100) {
			file << "0" << +c;
		} else {
			file << +c;
		}
	}
	file << endl;
}

void write_secrets(const string& fname) {
	ofstream file(fname, std::ios::app);
	if(!file) {
		cerr << "Could not open file" << endl;
	}

	cout << "  enter a key: "; 
	string master; 
	getline(cin, master); 

	cout << "  enter phrase to encrypt: "; 
	string pass; 
	getline(cin, pass); 

	cout << "  optionally, enter a non-integer tag: "; 
	string tag; 
	getline(cin, tag); 

	if(pass.empty()) {
		cerr << "Can't write - phrase is empty" << endl << endl;
	} else {
		string enc = encrypt(pass, master);
		file << tag << endl;
		write_as_nums(file, enc);
		cout << "  Done" << endl << endl;
	}
	file.close();
}

void delete_secrets(const string& fname, const string& target_tag, const int target_idx) {
	ifstream file(fname);
	if(!file) {
		cerr << "  Could not open file" << endl << endl;
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

int main() {
	print_intro();
	string homepath = getenv("HOME");
	string fname(homepath + "/.secret.txt");
	test_path(fname);

	while(1) {
		string target_tag; // c++ didn't like this in the switch statement
		int target_idx=-1; 
		cout << "  enter command (r/w/f/x/p/q/h): ";
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

