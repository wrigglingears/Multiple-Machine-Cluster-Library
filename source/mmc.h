/* Copyright (C) 2016
 * Contributed by Mattheus Lee <cs.mattheus.lee@gmail.com>
 */   
 
/* TODO
 *
 */
   
#ifndef MMC_H
#define MMC_H

#include <vector>
#include <string>

using namespace std;

#define NO_MANAGER                  -200

#define REQUESTING_TAG               100
#define WORK_TAG                     101
#define WORK_FLAG_TAG                102
#define RESULTS_TAG                  103

/* This library uses vector<vector<int>>
 * to hold the layout of a cluster.
 */
typedef vector<vector<int>> Layout_t;

class MMC_Machine {
    protected:
    Layout_t layout_;
    int rank_;
    char name_[20];
    string sName_;
    int nameLength_;
    int nThreads_;
    string role_;
    int level_;
    int managerRank_;
    int rankInLevel_;
    bool isTop_;
    
    void actual_MMC_Machine_ctor(Layout_t&);
    MMC_Machine();
    
    public:
    MMC_Machine(Layout_t&);
    ~MMC_Machine();
    
    Layout_t layout();
    int rank();
    string name();
    int name_length();
    int n_threads();
    string role();
    int level();
    int manager_rank();
    int rank_in_level();
    bool is_top();
    
    //void print_layout_info();
    //void print_comm_info();
};

class MMC_Thread : public MMC_Machine {
    private:
    int threadID_;
    
    public:
    MMC_Thread(Layout_t&);
    MMC_Thread();
    ~MMC_Thread();
    MMC_Thread& operator=(MMC_Machine&);
    
    int thread_id();
};

class MMC_Worker : public MMC_Machine {
    private:
    
    public:
    MMC_Worker(Layout_t&);
    MMC_Worker();
    ~MMC_Worker();
    MMC_Worker& operator=(MMC_Machine&);
    
    bool get_more_work();
};

class MMC_Manager : public MMC_Worker {
    private:
    int nWorkers_;
    vector<int> workerRanks_;
    int lastSentWorkRank_;
    
    void send_more_work_flag(int);
    void send_end_work_flag(int);
    
    public:
    MMC_Manager(Layout_t&);
    MMC_Manager();
    ~MMC_Manager();
    MMC_Manager& operator=(MMC_Machine&);
    
    int n_workers();
    int first_worker();
    int last_worker();
    int nth_worker(int);
    int worker_index_from_rank(int);
    int next_worker();
    void clear_worker_queue();
};

class MMC_Lock {
	private:
	pthread_mutex_t lock_;

	public:
	MMC_Lock();
	~MMC_Lock();

	void lock();
	void unlock();
};

Layout_t read_cluster_layout_from_file();
Layout_t read_cluster_layout_from_file(string&);

#endif //MMC_H_INCLUDED
