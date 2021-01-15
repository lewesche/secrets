#ifndef SECRETS
#define SECRETS

class secret {
private:
	std::string key;
	std::string enc_nums; // encrypted text as a string of 3 digit nums, 0-256
	std::string enc; // in normal char form
	std::string dec; // decrypted "plain" text

public: 
	// set_enc xpects a string of numbers, where each 3 ranges from 0-256
	void set_enc(const std::string& e); 
	void set_dec(const std::string& d);
	void set_key(const std::string& k);

	const std::string& get_enc() const; 
	const std::string& get_dec() const;
	const std::string& get_key() const;


	void encrypt();
	void decrypt();

	// Writing to the output file as nums instead of single characters 
	// 1/3 as efficient as writing the actual chars, but saves needing
	// to handle the case where the encrypted text forms a \n
	//
	// Disks are big
	void write(std::ofstream& file) const;

private:
	std::string shift(const std::string& phrase, int multiplier) const;

	// Converts a string of 3-char numbers (0-256) to a string of chars
	std::string numstr_2_charstr(const std::string& numstr) const;
};

#endif

