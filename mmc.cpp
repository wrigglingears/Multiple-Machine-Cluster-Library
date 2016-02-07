#include "mmc.h"

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <omp.h>
#include <mpi.h>
#include <pthread.h>
#include <sstream>
#include <string>
#include <vector>

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
        cout << "fatal error - no information in layout" << endl;
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
    for (unsigned int i = 0; i < layout_.size() && !found; ++i) {
        for (unsigned int j = 0; j < layout[i].size() && !found; ++j) {
            if(layout[i][j] == rank_) {
                found = true;
                level_ = i;
                rankInLevel_ = j;
            }
        }
    }
    if (!found) {
        cout << "fatal error - machine rank not found in layout" << endl;
        throw "fatal error - machine rank not found in layout";
    }
    if (level_ == 0) {
        role_ = "worker";
    }
    else {
        role_ = "manager";
        if (level_ == layout_.size() - 1) {
            isTop_ = true;    
        }
        else {
            isTop_ = false;
        }
    }
    if ((unsigned int)level_ == layout.size() - 1) {
        managerRank_ = NO_MANAGER;
    }
    else {
        int nInGroup = layout[level_].size() / layout[level_ + 1].size();
        int accumulate = nInGroup;
        for (int i = 0; ; ++i) {
            if (rankInLevel_ < accumulate) {
                managerRank_ = layout[level_ + 1][i];
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
 * will try to run.
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

/* Accessor for the rank of the machine's manager.
 * Takes in no arguments, and returns an integer. 
 */
int MMC_Machine::get_manager_rank() {
    return managerRank_;
}

/* Accessor for the rank of the machine within its level.
 * Takes in no arguments, and returns an integer. 
 */
int MMC_Machine::get_rank_in_level() {
    return rankInLevel_;
}

/* Accessor for whether the machine is in the top level.
 * Takes in no arguments, and returns a boolean.
 */
bool MMC_Machine::is_top() {
    return isTop_;    
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
    cout << " manager rank: " << managerRank_ << endl;
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
 * Takes in a no arguments. Should be followed
 * by the = operation to initialize members.
 */
MMC_Thread::MMC_Thread() {
}

/* Destructor for the MMC_Thread class.
 */
MMC_Thread::~MMC_Thread() {
}

/* More efficient initialization of the MMC_Thread
 * object, using an already created MMC_Machine
 * object.
 */
MMC_Thread& MMC_Thread::operator=(MMC_Machine& machine) {
    if (this == &machine) {
        return *this;
    }
    layout_ = machine.get_layout();
    rank_ = machine.get_rank();
    sName_ = machine.get_name();
    nameLength_ = machine.get_name_length();
    nThreads_ = machine.get_n_threads();
    role_ = machine.get_role();
    level_ = machine.get_level();
    managerRank_ = machine.get_manager_rank();
    rankInLevel_ = machine.get_rank_in_level();
    threadID_ = omp_get_thread_num();
    return *this;
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
 * Takes in no arguments. Should be followed
 * by the = operation to initialize members.
 */
MMC_Worker::MMC_Worker() {
}

/* Destructor for the MMC_Worker class.
 */
MMC_Worker::~MMC_Worker() {
}

/* More efficient initialization of the MMC_Worker
 * object, using an already created MMC_Machine
 * object.
 */
MMC_Worker& MMC_Worker::operator=(MMC_Machine& machine) {
    if (this == &machine) {
        return *this;
    }
    layout_ = machine.get_layout();
    rank_ = machine.get_rank();
    sName_ = machine.get_name();
    nameLength_ = machine.get_name_length();
    nThreads_ = machine.get_n_threads();
    role_ = machine.get_role();
    level_ = machine.get_level();
    managerRank_ = machine.get_manager_rank();
    rankInLevel_ = machine.get_rank_in_level();
    return *this;
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
    MPI_Isend(&sendDummy, 1, MPI_INT, managerRank_, REQUESTING_TAG, MPI_COMM_WORLD, &requestDummy);
    int workFlag;
    MPI_Recv(&workFlag, 1, MPI_INT, managerRank_, WORK_FLAG_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);         
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
    nWorkers_ = layout_[level_ - 1].size() / layout_[level_].size();
    int start = nWorkers_ * rankInLevel_, end = start + nWorkers_;
    for (int i = start; i < end; ++i) {
        workerRanks_.push_back(layout_[level_ - 1][i]);
    }
}

/* Constructor for the MMC_Manager class.
* Takes in no arguments. Should be followed
* by the = operation to initialize members.
*/
MMC_Manager::MMC_Manager() {
}

/* Destructor for the MMC_Manager class. 
*/
MMC_Manager::~MMC_Manager() {
}

/* More efficient initialization of the MMC_Manager
 * object, using an already created MMC_Machine
 * object.
 */
MMC_Manager& MMC_Manager::operator=(MMC_Machine& machine) {
    if (this == &machine) {
        return *this;
    }
    layout_ = machine.get_layout();
    rank_ = machine.get_rank();
    sName_ = machine.get_name();
    nameLength_ = machine.get_name_length();
    nThreads_ = machine.get_n_threads();
    role_ = machine.get_role();
    level_ = machine.get_level();
    managerRank_ = machine.get_manager_rank();
    rankInLevel_ = machine.get_rank_in_level();
    nWorkers_ = layout_[level_ - 1].size() / layout_[level_].size();
    int start = nWorkers_ * rankInLevel_;
    int end = start + nWorkers_;
    for (int i = start; i < end; ++i) {
        //cout << "pushing back with " << " i: " << i << " --- " << layout_[level_ - 1][i] <<  endl;
        workerRanks_.push_back(layout_[level_ - 1][i]);
    }
    return *this;
}

/* Accessor for the number of workers under 
 * the manager.
 * Takes in no aruments, and returns an integer.
 */
int MMC_Manager::get_n_workers() {
    return nWorkers_;;
}

/* Accessor for the rank of the first worker
 * under the manager.
 * Takes in no arguments, and returns an integer.
 */
int MMC_Manager::get_first_worker() {
    return workerRanks_[0];
}

/* Accessor for the rank of the last worker 
 * under the manager.
 * Takes in no arguments, and returns an integer.
 */
int MMC_Manager::get_last_worker() {
    return workerRanks_[nWorkers_ - 1];
}

/* Accessor for the rank of the nth worker 
 * under the manager.
 * Takes in an integer argument, and returns an integer.
 */
int MMC_Manager::get_nth_worker(int n) {
    try {
        if (n > nWorkers_) {
            cout << "fatal error - manager tried to access worker that doesn't exist" << endl;
            throw "fatal error - manager tried to access worker that doesn't exist";
        }
    }
    catch (const string errorMessage) {
        cout << "ERROR: " << errorMessage << endl;
        assert(0);
    }
    return workerRanks_[n];
}

/* Method to get the next rank to send work to.
 * The method call should be followed up by sending the
 * work to the worker rank.
 * Takes in no arguments, and returns an integer.
 */
int MMC_Manager::get_next_worker() {
    int startRank = layout_[level_ - 1][0], lastRank = layout_[level_ - 1][layout_[level_ - 1].size() - 1];
    for (int commRank = startRank; commRank <= lastRank; ++commRank) {
        int flag;
        MPI_Iprobe(commRank, REQUESTING_TAG, MPI_COMM_WORLD, &flag, MPI_STATUS_IGNORE);
        if (flag) {
            int recvDummy;
            MPI_Recv(&recvDummy, 1, MPI_INT, commRank, REQUESTING_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
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
    int startRank = layout_[level_ - 1][0], lastRank = layout_[level_ - 1][layout_[level_ - 1].size() - 1];
    
    for (int i = startRank; i <= lastRank; ++i) {
        send_end_work_flag(i);
    }
}

/* Method to inform a worker that there is more 
 * work for it to do.
 * Takes in an integer value, and returns no value. 
 */
void MMC_Manager::send_more_work_flag(int commRank) {
    int sendFlag = 1;
    MPI_Send(&sendFlag, 1, MPI_INT, commRank, WORK_FLAG_TAG, MPI_COMM_WORLD);
}

/* Method to inform a worker that there is no more 
 * work for it to do.
 * Takes in an integer value, and returns no value. 
 */
void MMC_Manager::send_end_work_flag(int commRank) {
    int sendFlag = 0;
    MPI_Send(&sendFlag, 1, MPI_INT, commRank, WORK_FLAG_TAG, MPI_COMM_WORLD);
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
            cout << "fatal error - no information in layout" << endl;
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
