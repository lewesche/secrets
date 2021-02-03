#include <iostream>
#include <string>
#include <fstream>
#include "secret.h"
#include <pthread.h>
#include <unordered_set>

using namespace std;

unordered_set<pthread_t> thds;	// to keep track of threads to join with
pthread_mutex_t set_lock; 		// to lock the thds set
pthread_mutex_t file_lock;		// to lock reading/writing to file or cout (when necessary)
pthread_cond_t cv;				// to ensure threads print out secrets by index order

void wait_for_thds() {
	pthread_mutex_lock(&set_lock);
	unordered_set<pthread_t>::iterator it;
	for(it = thds.begin(); it!=thds.end(); it++) {
		pthread_join(*it, nullptr);
	}
	pthread_mutex_unlock(&set_lock);
	thds.clear();
}

// Try and remove thread from the set of threads
// Can't? No big deal, that means the main thd is already 
// waiting to join. Erasing would mess up the iterator
void tryremove() {
	if(pthread_mutex_trylock(&set_lock)==0) {
		if(thds.find(pthread_self()) != thds.end())
			thds.erase(pthread_self());
		pthread_mutex_unlock(&set_lock);
	}
}

void* thd_read(void *arg) {
	secret *s = (secret *)arg;
	s->decrypt();
	pthread_mutex_lock(&file_lock);
	while(*(s->get_last_printed())+1 != s->get_idx()) {
		pthread_cond_wait(&cv, &file_lock);
	}

	cout << "  " << s->get_idx() << '\t' << s->get_tag() << endl;
	cout << "    " << '\t' << s->get_dec() << endl;
	*(s->get_last_printed()) = s->get_idx();
	pthread_cond_broadcast(&cv);
	pthread_mutex_unlock(&file_lock);

	delete s;
	tryremove();
	return nullptr;
}

// Reads secret based on tag or idx
// Reads all secrets if target_idx = -1 and target_tag is empty
void read_secrets(const string& fname, const string& target_tag, const int target_idx) {
	cout << "  enter a key: "; 
	string key;
	getline(cin, key); 

	wait_for_thds();

	// No need to lock here, only main thread will be reading from file
	// Instead, threads will use the lock to share cout
	ifstream file(fname);
	if(!file) {
		cerr << "  Could not open file" << endl;
		return;
	}

	cout << "  " << "idx" << '\t' << "tag" << endl;
	cout << "    " << '\t' << "seceret" << endl;

	size_t i=0;
	int last_printed = -1;
	while(!file.eof()) {
		string tag;
		getline(file, tag);
		string enc;
		getline(file, enc);
		if(!enc.empty()) {
			if((target_tag.empty() && target_idx==-1) || (!target_tag.empty() && target_tag==tag) || target_idx==i) {
				secret *s = new secret;
				s->set_key(key);
				s->set_enc(enc);
				s->set_tag(tag);
				s->set_idx(i);
				s->set_last_printed(&last_printed);
				
				pthread_mutex_lock(&set_lock);
				pthread_t thread;
				pthread_create(&thread, NULL, thd_read, (void*)s);
				thds.insert(thread);
				pthread_mutex_unlock(&set_lock);

			}
			++i;
		}
	}
	wait_for_thds();
	cout << endl;
	file.close();
}

void* thd_write(void *arg) {
	secret *s = (secret *)arg;
	s->encrypt();
	pthread_mutex_lock(&file_lock);
	s->write();
	pthread_mutex_unlock(&file_lock);

	delete s;
	tryremove();
	return nullptr;
}

void write_secrets(const string& fname) {
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
		secret *s = new secret;
		s->set_key(key);
		s->set_dec(dec);
		s->set_fname(fname);
		s->set_tag(tag);

		pthread_mutex_lock(&set_lock);
		pthread_t thread;
		pthread_create(&thread, NULL, thd_write, (void*)s);
		// add the new thread to the set of working threads
		// to be (possibly) joined on
		thds.insert(thread);
		pthread_mutex_unlock(&set_lock);

		cout << endl;
	}
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

	set_lock = PTHREAD_MUTEX_INITIALIZER;
	file_lock = PTHREAD_MUTEX_INITIALIZER;
	cv = PTHREAD_COND_INITIALIZER;
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
				wait_for_thds();
				return 0;
		}
	}
}

