#include <iostream>
#include <string>
#include <fstream>
#include "secret.h"
#include <math.h>

using namespace std;

void secret::set_enc(const std::string& e) {
	enc_nums = e;
	enc = numstr_2_charstr(enc_nums); 
}

void secret::set_dec(const std::string& d) {
	dec = d;
}

void secret::set_key(const std::string& k) {
	key = k;
}

void secret::set_fname(const std::string& f) {
	fname = f;
}

void secret::set_tag(const std::string& t) {
	tag = t;
}

void secret::set_idx(size_t i) {
	idx = i;
}

const string& secret::get_enc() const {
	return enc;
}

const string& secret::get_dec() const {
	return dec;
}

const string& secret::get_key() const {
	return key;
}

const string& secret::get_fname() const {
	return fname;
}

const string& secret::get_tag() const {
	return tag;
}

size_t secret::get_idx() const {
	return idx;
}

bool secret::has_dec() const {
	return !dec.empty();
}

bool secret::has_tag() const {
	return !tag.empty();
}

string secret::shift(const string& phrase, int multiplier) const {
	// The multiplier input is pretty much arbitrary as long as 
	// the abs value of the multipliers used for encryption and
	// decryption match, and they are opposite signs. 
	//
	// But for some bonus security lets factor in the key size
	multiplier *= key.size()+1;

	// And the sum of all key chars
	multiplier *= strsum(key)+1;

	// Some complicated math to figure out the num passes
	size_t min_passes = 1 + static_cast<size_t>((hypot(key.size(), strsum(key)) + (pow(static_cast<double>(strsum(key)), static_cast<double>(key.size()))))) % MAX_PASSES;

	size_t phrase_pass_count = 0, key_pass_count = 0;

	size_t i=0;
	size_t j=0;
	bool done_phrase_passes = false;
	bool done_key_passes = false;
	string res = phrase;

	// Make sure to iterate over the both strings entierly
	while(!done_phrase_passes || !done_key_passes) {
		if(j>=key.size()) {
			j=0;
			++key_pass_count;
			if(key_pass_count >= min_passes)
				done_key_passes = true;
		}
		if(i>=phrase.size()) {
			i=0;
			++phrase_pass_count;
			if(phrase_pass_count >= min_passes)
				done_phrase_passes = true;
		}

		res[i] = res[i] + key[j]*multiplier; 
		++j;
		++i;
	}
	return res;
}

void secret::encrypt() {
	enc = shift(dec, MULTIPLIER);
	// enc_nums might be out of date if dec changed, but doesn't matter
}

void secret::decrypt() {
	dec = shift(enc, -1*MULTIPLIER);
	check_dec();
}

void secret::check_dec() {
	for(size_t i=0; i<dec.size(); ++i) {
		if(dec[i] <= 0x1f || dec[i] >= 0x7f) 
			dec[i] = '?';
	}	
}

char secret::strsum(string s) const {
	char res = 0;
	for(size_t i=0; i<s.size(); ++i) {
		res += s[i];
	}
	return res;
}

string secret::numstr_2_charstr(const string& numstr) const {
	string charstr;
	for(size_t i=0; i<numstr.length(); i+=3) {
		string substr;
		substr += numstr[i];
		substr += numstr[i+1];
		substr += numstr[i+2];
		char c = stoi(substr);	
		charstr += c;
	}
	return charstr;
}

int secret::write() const {
	ofstream file(fname, std::ios::app);
    if(!file) {
        cerr << "Could not open file" << endl;
        return -1;
    }

	file << tag << endl;	

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
	file.close();
	return 0;
}

