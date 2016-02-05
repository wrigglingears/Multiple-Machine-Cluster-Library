#ifndef MMC_H
#define MMC_H

using namespace std;

#define NO_REPORTING_RANK            -200

#define REQUESTING_WORK_TAG          100
#define RECEIVING_WORK_TAG           101
#define RECEIVING_WORK_FLAG_TAG      102
#define SENDING_RESULTS_TAG          103

/* These barriers are useful for creating synchronization points.
 * Use MMC_machine_barrier to synchronize within a machine,
 * and MMC_program_barrier to synchronize within the entire program.
 * It is not recommended to call MMC_program_barrier while inside
 * a threaded portion.
 */
#define _mmc_machine_barrier_ 	     _Pragma("omp barrier")
#define _mmc_program_barrier_	     MPI_Barrier(MPI_COMM_WORLD);

/* In this library the vector<vector<int>> type
 * is type-defined as Layout.
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
    int reportingRank_;
    
    void actual_MMC_Machine_ctor(Layout_t&);
    
    public:
    MMC_Machine(Layout_t&);
    ~MMC_Machine();
    
    Layout_t get_layout();
    int get_rank();
    string get_name();
    int get_name_length();
    int get_n_threads();
    string get_role();
    int get_level();
    int get_reporting_rank();
    
    void print_layout_info();
    void print_comm_info();
};

class MMC_Thread : public MMC_Machine {
    private:
    int threadID_;
    
    public:
    MMC_Thread(MMC_Machine&);
    MMC_Thread(Layout_t&);
    ~MMC_Thread();
    
    int get_thread_id();
};

class MMC_Worker : public MMC_Machine {
    private:
    
    public:
    MMC_Worker(MMC_Machine&);
    MMC_Worker(Layout_t&);
    ~MMC_Worker();
    
    bool get_more_work();
};

class MMC_Manager : public MMC_Worker {
    private:
    
    public:
    MMC_Manager(MMC_Machine&);
    MMC_Manager(Layout_t&);
    ~MMC_Manager();
    
    int get_n_workers();
    int get_first_worker();
    int get_last_worker();
    int get_nth_worker(int);
    int get_send_work_rank();
    void clear_worker_queue();
    
    void send_more_work_flag(int);
    void send_end_work_flag(int);
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

void MMC_Init();
void MMC_Finalize();

Layout_t read_cluster_layout_from_file();

double get_wtime();

#endif //MMC_H
