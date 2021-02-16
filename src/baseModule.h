#ifndef BASE_MODULE
#define BASE_MODULE

#include <queue>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>

#define MAX_THREADS 8

class BaseModule {
private:
	std::thread thds[MAX_THREADS];
	std::mutex queue_lock; 				// used to lock jobs queue
	std::queue<secret*> jobs;

public:
	std::string fname;

	//base_read allocated memory
	std::vector<secret*> base_read(const std::string& key, const std::string& target_tag, const int target_idx);
	void base_write(const std::string& key, const std::string& dec, const std::string& tag);
	void list_secrets();
	void delete_secrets(const std::string& target_tag, const int target_idx);

	virtual void read_secrets(const std::string& target_tag, const int target_idx) = 0;
	virtual void write_secrets() = 0;

	void test_path();

	void printSecrets(std::vector<secret*>& vec);
	void printJson(std::vector<secret*>& vec);

private:
	void thd_read();
	// called to synchronize before and after reading  
	void wait_for_thds(int num_thds);
};



#endif
