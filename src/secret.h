#ifndef SECRET
#define SECRET


/*  ~~~ WARING ~~~
	MULTIPLIER and MAX_PASSES are two constants used during encryption/decryption. 
	
	They can be changed for extra securty. 
	Changing either constant before compiling will result in a .secrets.txt file that cannot be read by other binaries compiled with different constants. 
	MAX_PASSES also effects the speed of the program, larger MAX_PASSES == slower decryption/encryption. 
*/

#define MULTIPLIER 5
#define MAX_PASSES 8529

class secret {
private:
	std::string key;
	std::string enc_nums; // encrypted text as a string of 3 digit nums, 0-256
	std::string enc; // in normal char form
	std::string dec; // decrypted "plain" text
	std::string fname; // filename for writing
	std::string tag;
	size_t idx;

public: 
	// set_enc xpects a string of numbers, where each 3 ranges from 0-256
	void set_enc(const std::string& e); 
	void set_dec(const std::string& d);
	void set_key(const std::string& k);
	void set_fname(const std::string& f);
	void set_tag(const std::string& t);
	void set_idx(size_t i);

	const std::string& get_enc() const; 
	const std::string& get_dec() const;
	const std::string& get_key() const;
	const std::string& get_fname() const;
	const std::string& get_tag() const;
	size_t get_idx() const;

	void encrypt();
	void decrypt();

	// Writing to the output file as nums instead of single characters 
	// 1/3 as efficient as writing the actual chars, but saves needing
	// to handle the case where the encrypted text forms a \n
	//
	// Disks are big
	void write() const;

private:
	std::string shift(const std::string& phrase, int multiplier) const;
	char strsum(std::string s) const;

	// Converts a string of 3-char numbers (0-256) to a string of chars
	std::string numstr_2_charstr(const std::string& numstr) const;
};

#endif

