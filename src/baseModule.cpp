#include <iostream>
#include <string>
#include <fstream>
#include "secret.h"
#include "baseModule.h"
#include <queue>
#include <thread>
#include <mutex>

using namespace std;

void BaseModule::wait_for_thds(int num_thds) {
	for (int i=0; i<num_thds; i++){
        thds[i].join();
    }
}

void BaseModule::thd_read() {
	while(!jobs.empty()) {
		queue_lock.lock();
		if(jobs.empty())
			return;
		secret *s = jobs.front();
		jobs.pop();
		queue_lock.unlock();
    	s->decrypt();
	}
	return;
}

// Reads secret based on tag or idx
// Reads all secrets if target_idx = -1 and target_tag is empty
vector<secret*> BaseModule::base_read(const string& key, const string& target_tag, const int target_idx) {
    // No need to lock here, only main thread will be reading from file
    // Instead, threads will use the lock to share cout
    ifstream file(fname);
	vector<secret*> res;
    if(!file) {
        cerr << "  Could not open file" << endl;
        return res;
    }

	// build queue of jobs
    size_t i=0;
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
				jobs.push(s);
				res.push_back(s);
            }
            ++i;
        }
    }

	//spawn threads
	int num_thds = (jobs.size() < MAX_THREADS) ? jobs.size() : MAX_THREADS;
	for(size_t t=0; t<num_thds; ++t) {
		thds[t] = thread([this](){ thd_read(); });	
	}

    wait_for_thds(num_thds);
    file.close();
	return res;
}

void BaseModule::base_write(const string& key, const string& dec, const string& tag) {
    if(dec.empty()) {
        cerr << "Can't write - phrase is empty" << endl << endl;
    } else {
        secret *s = new secret;
        s->set_key(key);
        s->set_dec(dec);
        s->set_fname(fname);
        s->set_tag(tag);
		s->encrypt();
		s->write();
		delete s;

        cout << endl;
    }
}

void BaseModule::print_secrets(vector<secret*>& vec) {
    for(size_t i=0; i<vec.size(); ++i) {
        secret *s = vec[i];
        cout << "  " << s->get_idx() << '\t' << s->get_tag() << endl;
		if(s->has_dec())
        	cout << "    " << '\t' << s->get_dec() << endl;
        delete s;
    }
    cout<<endl;
}

void BaseModule::print_secrets_json(vector<secret*>& vec) {
	cout << "[";

    for(size_t i=0; i<vec.size(); ++i) {
        secret *s = vec[i];
		if(i!=0)
			cout << ",";
		cout << "{\"idx:\":\"" << s->get_idx(); 
		if(s->has_tag())
			cout << "\",\"tag\":\"" << s->get_tag();
		if(s->has_dec())
			cout << "\",\"dec\":\"" << s->get_dec();
		cout << "\"}";
        delete s;
    }
    cout << "]" << endl;
}

vector<secret*> BaseModule::base_list(const string& target_tag, const int target_idx) {
	vector<secret*> res;
    ifstream file(fname);
    if(!file) {
        cerr << "  Could not open file" << endl << endl;
        return res;
    }

 	size_t i=0;
    while(!file.eof()) {
        string tag;
        getline(file, tag);
        string enc;
        getline(file, enc); // not used, but need to skip the line
        if(!enc.empty()) {
            if((target_tag.empty() && target_idx==-1) || (!target_tag.empty() && target_tag==tag) || target_idx==i) {
                secret *s = new secret;
                s->set_tag(tag);
                s->set_idx(i);
				res.push_back(s);
            }
            ++i;
        }
    }

	/*
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
	*/
    file.close();
	return res;
}

void BaseModule::base_delete(const string& target_tag, const int target_idx) {
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

void BaseModule::test_path() {
    ifstream ifile(fname);
    if(!ifile){
        //cout << "  Couldn't find " << fname << ", creating file." << endl << endl;
        ofstream ofile(fname);
    } else {
        ifile.close();
    }
}
