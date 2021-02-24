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
	int base_write(const std::string& key, const std::string& dec, const std::string& tag);
	std::vector<secret*> base_list(const std::string& target_tag, const int target_idx);
	int base_delete(const std::string& target_tag, const int target_idx);

	virtual void read_secrets() = 0;
	virtual void write_secrets() = 0;
	virtual void list_secrets() = 0;
	virtual void delete_secrets() = 0;

	void test_path();

	void print_secrets(std::vector<secret*>& vec);
	void print_secrets_json(std::vector<secret*>& vec);
	void print_success();
	void print_success_json();
	void print_failure();
	void print_failure_json();

private:
	void thd_read();
	// called to synchronize before and after reading  
	void wait_for_thds(int num_thds);
};



#endif
