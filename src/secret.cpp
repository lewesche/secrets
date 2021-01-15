#include <iostream>
#include <string>
#include <fstream>
#include "secret.h"

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

const string& secret::get_enc() const {
	return enc;
}

const string& secret::get_dec() const {
	return dec;
}

const string& secret::get_key() const {
	return key;
}


string secret::shift(const string& phrase, int multiplier) const {
	size_t i=0;
	size_t j=0;
	bool seen_full_phrase = false;
	bool seen_full_key = false;
	string res = phrase;

	// The multiplier input is pretty much arbitrary as long as 
	// the abs value of the multipliers used for encryption and
	// decryption match, and they are opposite signs. 
	//
	// But for some bonus security lets factor in the key size
	multiplier *= key.size();

	// Make sure to iterate over the both strings entierly
	while(!seen_full_phrase || !seen_full_key) {
		if(j>=key.size()) {
			j=0;
			seen_full_key = true;
		}
		if(i>=phrase.size()) {
			i=0;
			seen_full_phrase = true;
		}

		res[i] = res[i] + key[j]*multiplier; 
		++j;
		++i;
	}
	return res;
}

void secret::encrypt() {
	enc = shift(dec, 5);
	// enc_nums might be out of date if dec changed, but doesn't matter
}

void secret::decrypt() {
	dec = shift(enc, -5);
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

void secret::write(ofstream& file) const {
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

