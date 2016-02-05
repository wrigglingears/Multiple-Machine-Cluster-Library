#include <iostream>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <cassert>
#include <omp.h>
#include <mpi.h>
#include <pthread.h>
#include "mmc.h"

using namespace std;

///////////////////////////////////
/* MMC_Machine class starts here */
///////////////////////////////////

/* This is the functional constructor method for the
 * MMC_Machine class. It is not the method called by the 
 * user, instead the user calls a wrapper around this method
 * that checks for exceptions in the process.
 */
void MMC_Machine::actual_MMC_Machine_ctor(Layout_t& layout) {
    if (layout_.size() == 0) {
        throw "fatal error - no information in layout";
    }
    else if (layout_.size() == 1) {
        cout << "WARNING: cluster is only arranged on one level." << endl;
        cout << "Unexpected performance may result." << endl;
    }
    MPI_Comm_rank(MPI_COMM_WORLD, &rank_);
    MPI_Get_processor_name(name_, &nameLength_);
    sName_ = string(name_);
    ifstream inputFile("/home/mattheus/machine_private/machine.threads");
    inputFile >> nThreads_;
    inputFile.close();
    bool found = false;
    int posInGroup;
    for (unsigned int i = 0; i < layout_.size() && !found; ++i) {
        for (unsigned int j = 0; j < layout[i].size() && !found; ++j) {
            if(layout[i][j] == rank_) {
                found = true;
                level_ = i;
                posInGroup = j;
            }
        }
    }
    if (!found) {
        throw "fatal error - machine rank not found in layout";
    }
    if (level_ == 0) {
        role_ = "worker";
    }
    else {
        role_ = "manager";
    }
    if ((unsigned int)level_ == layout.size() - 1) {
        reportingRank_ = NO_REPORTING_RANK;
    }
    else {
        int nInGroup = layout[level_].size() / layout[level_ + 1].size();
        int accumulate = nInGroup;
        for (int i = 0; ; ++i) {
            if (posInGroup < accumulate) {
                reportingRank_ = layout[level_ + 1][i];
                break;
            }
            accumulate += nInGroup;
        }
    }
}

/* Contructor for the MMC_Machine class.
 * It wraps around the actual_MMC_Machine_ctor method,
 * catching any exceptions that are thrown.
 * This constructor takes in a Layout_t variable detailing the
 * cluster layout.
 */
MMC_Machine::MMC_Machine(Layout_t& layout) 
            :layout_(layout) {
    try {
        actual_MMC_Machine_ctor(layout);
    }
    catch (const string errorMessage) {
        cout << "ERROR: " << errorMessage << endl;
        assert(0);
    }
}

/* Destructor for the MMC_Machine class.
 */
MMC_Machine::~MMC_Machine() {
}

/* Accessor for the layout of the cluster.
 * Takes in no arguments, and returns a Layout_t variable. 
 */
Layout_t MMC_Machine::get_layout() {
    return layout_;
}

/* Accessor for the rank of the machine.
 * Takes in no arguments, and returns an integer.
 */
int MMC_Machine::get_rank() {
    return rank_;
}

/* Accessor for the name of the machine.
 * Takes in no arguments, and returns a string.
 */
string MMC_Machine::get_name() {
    return sName_;
}

/* Accessor for the length of the name of the
 * machine.
 * Take in no arguments, and returns an integer.
 */
int MMC_Machine::get_name_length() {
    return nameLength_;
}

/* Accessor for the number of threads the machine
 * supports.
 * Takes in no arguments, and returns an integer.
 */
int MMC_Machine::get_n_threads() {
    return nThreads_;
}

/* Accessor for the role of the machine.
 * Takes in no arguments, and returns a string.
 */
string MMC_Machine::get_role() {
    return role_;
}

/* Accessor for the level of the machine.
 * Takes in no arguments, and returns an integer.
 */
int MMC_Machine::get_level() {
    return level_;
}

/* Accessor for the reporting rank of the machine.
 * Takes in no arguments, and returns an integer. 
 */
int MMC_Machine::get_reporting_rank() {
    return reportingRank_;
}

void MMC_Machine::print_layout_info() {
    for (unsigned int i = 0; i < layout_.size(); ++i) {
        for (unsigned int j = 0; j < layout_[i].size(); ++j) {
            cout << layout_[i][j] << " ";
        }
        cout << endl;
    }
}

void MMC_Machine::print_comm_info() {
    cout << "rank: " << rank_;
    cout << " reporting rank: " << reportingRank_ << endl;
}

//////////////////////////////////
/* MMC_Thread class starts here */
//////////////////////////////////

/* Constructor for the MMC_Thread class.
 * Takes in a Layout_t variable detailing the cluster layout.
 */
MMC_Thread::MMC_Thread(Layout_t& layout)
           :MMC_Machine(layout) {
    threadID_ = omp_get_thread_num();               
}

/* Constructor for the MMC_Thread class.
 * Takes in an MMC_Machine object.
 * This constructor is preferred for speed
 * of execution, since is just copies over the 
 * data already in the MMC_Machine object.
 */
 /* COMMENTING OUT FOR NOW */
 /*
MMC_Thread::MMC_Thread(MMC_Machine& machine) {
    layout_ = machine.get_layout();
    rank_ = machine.get_rank();
    sName_ = machine.get_name();
    nameLength_ = machine.get_name_length();
    nThreads_ = machine.get_n_threads();
    role_ = machine.get_role();
    level_ = machine.get_level();
    reportingRank_ = machine.get_reporting_rank();
    threadID_ = omp_get_thread_num();
}
*/

/* Destructor for the MMC_Thread class.
 */
MMC_Thread::~MMC_Thread() {
}

/* Accessor for the thread ID of the thread.
 * Takes in no arguments, and returns an integer value.
 */
int MMC_Thread::get_thread_id() {
    return threadID_;
}

//////////////////////////////////
/* MMC_Worker class starts here */
//////////////////////////////////

/* Constructor for the MMC_Worker class.
 * Takes in a Layout_t variable detailing the cluster layout.
 */
MMC_Worker::MMC_Worker(Layout_t& layout)
           :MMC_Machine(layout) {               
}

/* Constructor for the MMC_Worker class.
 * Takes in an MMC_Machine object.
 * This constructor is preferred for speed
 * of execution, since is just copies over the 
 * data already in the MMC_Machine object.
 */
  /* COMMENTING OUT FOR NOW */
/*
MMC_Worker::MMC_Worker(MMC_Machine& machine) {
    layout_ = machine.get_layout();
    rank_ = machine.get_rank();
    sName_ = machine.get_name();
    nameLength_ = machine.get_name_length();
    nThreads_ = machine.get_n_threads();
    role_ = machine.get_role();
    level_ = machine.get_level();
    reportingRank_ = machine.get_reporting_rank();
}
*/

/* Destructor for the MMC_Worker class.
 */
MMC_Worker::~MMC_Worker() {
}

/* Method to check if there is more work that can be
 * received from this worker's manager.
 * Returns true if there is more work.
 * This method call should be followed up by receiving
 * the work from the worker's manager.
 * Takes in no arguments, and returns a boolean.
 */
bool MMC_Worker::get_more_work() {
    int sendDummy;
    MPI_Request requestDummy;
    MPI_Isend(&sendDummy, 1, MPI_INT, reportingRank_, REQUESTING_WORK_TAG, MPI_COMM_WORLD, &requestDummy);
    int workFlag;
    MPI_Recv(&workFlag, 1, MPI_INT, reportingRank_, RECEIVING_WORK_FLAG_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);         
    if (workFlag) {
        return true;
    }
    return false;
}

///////////////////////////////////
/* MMC_Manager class starts here */
///////////////////////////////////

/* Constructor for the MMC_Manager class.
 * Takes in a Layout_t variable detailing the 
 * cluster layout.
 */
MMC_Manager::MMC_Manager(Layout_t& layout)
           :MMC_Worker(layout) {               
}

/* Constructor for the MMC_Manager class.
 * Takes in an MMC_Machine object.
 * This constructor is preferred for speed
 * of execution, since is just copies over the 
 * data already in the MMC_Machine object.
 */
  /* COMMENTING OUT FOR NOW */
/*
MMC_Manager::MMC_Manager(MMC_Machine& machine) {
    layout_ = machine.get_layout();
    rank_ = machine.get_rank();
    sName_ = machine.get_name();
    nameLength_ = machine.get_name_length();
    nThreads_ = machine.get_n_threads();
    role_ = machine.get_role();
    level_ = machine.get_level();
    reportingRank_ = machine.get_reporting_rank();
}
*/

/* Destructor for the MMC_Manager class. 
 */
MMC_Manager::~MMC_Manager() {
}

/* Accessor for the number of workers under 
 * the manager.
 * Takes in no aruments, and returns an integer.
 */
int MMC_Manager::get_n_workers() {
    return layout_[level_ - 1].size();
}

/* Accessor for the rank of the first worker
 * under the manager.
 * Takes in no arguments, and returns an integer.
 */
int MMC_Manager::get_first_worker() {
    return layout_[level_ - 1][0];
}

/* Accessor for the rank of the last worker 
 * under the manager.
 * Takes in no arguments, and returns an integer.
 */
int MMC_Manager::get_last_worker() {
    return layout_[level_ - 1][layout_[level_ - 1].size() - 1];
}

/* Accessor for the rank of the nth worker 
 * under the manager.
 * Takes in an integer argument, and returns an integer.
 */
int MMC_Manager::get_nth_worker(int n) {
    return layout_[level_ - 1][0] + n;
}

/* Method to get the next rank to send work to.
 * The method call should be followed up by sending the
 * work to the worker rank.
 * Takes in no arguments, and returns an integer.
 */
int MMC_Manager::get_send_work_rank() {
    int startRank = layout_[level_ - 1][0], lastRank = layout_[level_ - 1][layout_[level_ - 1].size() - 1];
    for (int commRank = startRank; commRank <= lastRank; ++commRank) {
        int flag;
        MPI_Iprobe(commRank, REQUESTING_WORK_TAG, MPI_COMM_WORLD, &flag, MPI_STATUS_IGNORE);
        if (flag) {
            int recvDummy;
            MPI_Recv(&recvDummy, 1, MPI_INT, commRank, REQUESTING_WORK_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            send_more_work_flag(commRank);
            return commRank;
        }
        if (commRank == lastRank) {
            commRank = startRank - 1;
        }
    }
    return -1;
}

/* Method to ensure that all workers under the 
 * manager no longer expect any work units.
 * Takes in no arguments, and returns no value.
 */
void MMC_Manager::clear_worker_queue() {
    //int recvDummy;
    //int flag;
    //bool isDone = false;
    int startRank = layout_[level_ - 1][0], lastRank = layout_[level_ - 1][layout_[level_ - 1].size() - 1];
    
    for (int i = startRank; i <= lastRank; ++i) {
        send_end_work_flag(i);
    }
    /*
    while (!isDone) {
        isDone = true;
        for (int i = startRank; i <= lastRank; ++i) {
            MPI_Iprobe(i, REQUESTING_WORK_TAG, MPI_COMM_WORLD, &flag, MPI_STATUS_IGNORE);
            if (flag) {
                isDone = false;
                MPI_Recv(&recvDummy, 1, MPI_INT, i, REQUESTING_WORK_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                send_end_work_flag(i);
            }
        }
    }
    */
}

/* Method to inform a worker that there is more 
 * work for it to do.
 * Takes in an integer value, and returns no value. 
 */
void MMC_Manager::send_more_work_flag(int commRank) {
    int sendFlag = 1;
    MPI_Send(&sendFlag, 1, MPI_INT, commRank, RECEIVING_WORK_FLAG_TAG, MPI_COMM_WORLD);
}
/* Method to inform a worker that there is no more 
 * work for it to do.
 * Takes in an integer value, and returns no value. 
 */
void MMC_Manager::send_end_work_flag(int commRank) {
    int sendFlag = 0;
    MPI_Send(&sendFlag, 1, MPI_INT, commRank, RECEIVING_WORK_FLAG_TAG, MPI_COMM_WORLD);
}


////////////////////////////////
/* MMC_Lock class starts here */
////////////////////////////////

/* Constructor for the MMC_Lock class.
 */
MMC_Lock::MMC_Lock() {
    lock_ = PTHREAD_MUTEX_INITIALIZER;
}

/* Destructor for the MMC_Lock class.
 */
MMC_Lock::~MMC_Lock() {
}

/* Wrapper around pthread_mutex_lock.
 * Locks the lock for exclusive use.
 */
void MMC_Lock::lock() {
	pthread_mutex_lock(&lock_);
}

/* Wrapper around pthread_mutex_unlock.
 * Unlocks the lock, releasing it for
 * others to use. 
 */
void MMC_Lock::unlock() {
	pthread_mutex_unlock(&lock_);
}

/* Wrapper around MPI_Init. 
 * Initializes the cluster environment.
 */
void MMC_Init() {
    MPI_Init(NULL, NULL);
}

/* Wrapper around MPI_Finalize. 
 * Closes the cluster environment.
 * This should be the last function you call
 * before returning from main()
 */
void MMC_Finalize() {
    MPI_Finalize();
}

/* Reads the layout of the cluster from the file
 * cluster.layout, and arranges the data into a 
 * 2-dimensional vector which the user can treat as
 * an array.
 * Takes in no arguments, and returns a Layout_t variable.
 */
Layout_t read_cluster_layout_from_file() {
    ifstream layoutFile("cluster.layout");
    int numLayers;
    string layerString;
    getline(layoutFile, layerString);
    istringstream layerIss(layerString);
    try {
        layerIss >> numLayers;
        if (numLayers == 0) {
            throw "fatal error - no information in layout";
        }
        else if (numLayers == 1) {
            cout << "WARNING: cluster is only arranged on one level." << endl;
            cout << "Unexpected performance may result." << endl;
        }
    }
    catch(const string errorMessage) {
        cout << "ERROR: " << errorMessage << endl;
        assert(0);
    }
    catch(...) {
        cout << "ERROR: cannot get number of layers from file" << endl;
        assert(0);
    }
    Layout_t layout(numLayers);
    
    for (int i = 0; i < numLayers; ++i) {
        int readIn;
        getline(layoutFile, layerString);
        istringstream layerIss(layerString);
        while (layerIss >> readIn) {
            layout[i].push_back(readIn);
        }
    }
    layoutFile.close();
    return layout;
}

/* Wrapper around omp_get_wtime(), which returns
 * the time in seconds calculated from a fixed point.
 * Takes in no arguments, and returns an double value.
 */
double get_wtime() {
    return omp_get_wtime();
}
