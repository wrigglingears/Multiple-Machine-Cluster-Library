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
        cout << "fatal error - machine rank " << rank_ << " not found in layout" << endl;
        throw "fatal error - machine rank not found in layout";
    }
    if (level_ == 0) {
        role_ = "worker";
    }
    else {
        role_ = "manager";
        if ((unsigned int)level_ == layout_.size() - 1) {
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

MMC_Machine::MMC_Machine() {
}

/* Destructor for the MMC_Machine class.
 */
MMC_Machine::~MMC_Machine() {
}

/* Accessor for the layout of the cluster.
 * Takes in no arguments, and returns a Layout_t variable. 
 */
Layout_t MMC_Machine::layout() {
    return layout_;
}

/* Accessor for the rank of the machine.
 * Takes in no arguments, and returns an integer.
 */
int MMC_Machine::rank() {
    return rank_;
}

/* Accessor for the name of the machine.
 * Takes in no arguments, and returns a string.
 */
string MMC_Machine::name() {
    return sName_;
}

/* Accessor for the length of the name of the
 * machine.
 * Take in no arguments, and returns an integer.
 */
int MMC_Machine::name_length() {
    return nameLength_;
}

/* Accessor for the number of threads the machine
 * will try to run.
 * Takes in no arguments, and returns an integer.
 */
int MMC_Machine::n_threads() {
    return nThreads_;
}

/* Accessor for the role of the machine.
 * Takes in no arguments, and returns a string.
 */
string MMC_Machine::role() {
    return role_;
}

/* Accessor for the level of the machine.
 * Takes in no arguments, and returns an integer.
 */
int MMC_Machine::level() {
    return level_;
}

/* Accessor for the rank of the machine's manager.
 * Takes in no arguments, and returns an integer. 
 */
int MMC_Machine::manager_rank() {
    return managerRank_;
}

/* Accessor for the rank of the machine within its level.
 * Takes in no arguments, and returns an integer. 
 */
int MMC_Machine::rank_in_level() {
    return rankInLevel_;
}

/* Accessor for whether the machine is in the top level.
 * Takes in no arguments, and returns a boolean.
 */
bool MMC_Machine::is_top() {
    return isTop_;    
}

/* The following two methods were mainly for debugging purposes
 * and are not required in the actual implementation.
 */
/*
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
}*/

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

/* More efficient constructor for the MMC_Thread
 * object, using an already created MMC_Machine
 * object.
 */
MMC_Thread::MMC_Thread(MMC_Machine& machine) {
    layout_ = machine.layout();
    rank_ = machine.rank();
    sName_ = machine.name();
    nameLength_ = machine.name_length();
    nThreads_ = machine.n_threads();
    role_ = machine.role();
    level_ = machine.level();
    managerRank_ = machine.manager_rank();
    rankInLevel_ = machine.rank_in_level();
    threadID_ = omp_get_thread_num();
    isTop_ = machine.is_top();
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
    layout_ = machine.layout();
    rank_ = machine.rank();
    sName_ = machine.name();
    nameLength_ = machine.name_length();
    nThreads_ = machine.n_threads();
    role_ = machine.role();
    level_ = machine.level();
    managerRank_ = machine.manager_rank();
    rankInLevel_ = machine.rank_in_level();
    threadID_ = omp_get_thread_num();
    isTop_ = machine.is_top();
    return *this;
}

/* Accessor for the thread ID of the thread.
 * Takes in no arguments, and returns an integer value.
 */
int MMC_Thread::thread_id() {
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

/* More efficient constructor for the MMC_Worker
 * object, using an already created MMC_Machine
 * object.
 */
MMC_Worker::MMC_Worker(MMC_Machine& machine) {
    layout_ = machine.layout();
    rank_ = machine.rank();
    sName_ = machine.name();
    nameLength_ = machine.name_length();
    nThreads_ = machine.n_threads();
    role_ = machine.role();
    level_ = machine.level();
    managerRank_ = machine.manager_rank();
    rankInLevel_ = machine.rank_in_level();
    isTop_ = machine.is_top();
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
    layout_ = machine.layout();
    rank_ = machine.rank();
    sName_ = machine.name();
    nameLength_ = machine.name_length();
    nThreads_ = machine.n_threads();
    role_ = machine.role();
    level_ = machine.level();
    managerRank_ = machine.manager_rank();
    rankInLevel_ = machine.rank_in_level();
    isTop_ = machine.is_top();
    return *this;
}

/* Method to check if there is more work that can be
* received from this worker's manager.
* Returns true if there is more work.
* This method call should be followed up by receiving
* the work from the worker's manager.
* Takes in no arguments, and returns a boolean.
*/
bool MMC_Worker::request_more_work() {
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
    lastSentWorkRank_ = workerRanks_[0];
}

/* Constructor for the MMC_Manager class.
* Takes in no arguments. Should be followed
* by the = operation to initialize members.
*/
MMC_Manager::MMC_Manager() {
}

/* More efficient constructor for the MMC_Manager
 * object, using an already created MMC_Machine
 * object.
 */
MMC_Manager::MMC_Manager(MMC_Machine& machine) {
    layout_ = machine.layout();
    rank_ = machine.rank();
    sName_ = machine.name();
    nameLength_ = machine.name_length();
    nThreads_ = machine.n_threads();
    role_ = machine.role();
    level_ = machine.level();
    managerRank_ = machine.manager_rank();
    rankInLevel_ = machine.rank_in_level();
    nWorkers_ = layout_[level_ - 1].size() / layout_[level_].size();
    unsigned int start = nWorkers_ * rankInLevel_;
    unsigned int end = start + nWorkers_;
    if (end > layout_[level_ - 1].size()) {
        end = layout_[level_ - 1].size();
        nWorkers_ = end - start;
    }
    for (unsigned int i = start; i < end; ++i) {
        //cout << this->rank_ << " pushing back with " << " i: " << i << " --- " << layout_[level_ - 1][i] <<  endl;
        workerRanks_.push_back(layout_[level_ - 1][i]);
    }
    isTop_ = machine.is_top();
    lastSentWorkRank_ = workerRanks_[0];
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
    layout_ = machine.layout();
    rank_ = machine.rank();
    sName_ = machine.name();
    nameLength_ = machine.name_length();
    nThreads_ = machine.n_threads();
    role_ = machine.role();
    level_ = machine.level();
    managerRank_ = machine.manager_rank();
    rankInLevel_ = machine.rank_in_level();
    nWorkers_ = layout_[level_ - 1].size() / layout_[level_].size();
    unsigned int start = nWorkers_ * rankInLevel_;
    unsigned int end = start + nWorkers_;
    if (end > layout_[level_ - 1].size()) {
        end = layout_[level_ - 1].size();
        nWorkers_ = end - start;
    }
    for (unsigned int i = start; i < end; ++i) {
        //cout << this->rank_ << " pushing back with " << " i: " << i << " --- " << layout_[level_ - 1][i] <<  endl;
        workerRanks_.push_back(layout_[level_ - 1][i]);
    }
    isTop_ = machine.is_top();
    lastSentWorkRank_ = workerRanks_[0];
    return *this;
}

/* Accessor for the number of workers under 
 * the manager.
 * Takes in no aruments, and returns an integer.
 */
int MMC_Manager::n_workers() {
    return nWorkers_;;
}

/* Accessor for the rank of the first worker
 * under the manager.
 * Takes in no arguments, and returns an integer.
 */
int MMC_Manager::first_worker() {
    return workerRanks_[0];
}

/* Accessor for the rank of the last worker 
 * under the manager.
 * Takes in no arguments, and returns an integer.
 */
int MMC_Manager::last_worker() {
    return workerRanks_[nWorkers_ - 1];
}

/* Accessor for the rank of the nth worker 
 * under the manager.
 * Takes in an integer argument, and returns an integer.
 */
int MMC_Manager::nth_worker(int n) {
    try {
        if (n > nWorkers_) {
            cout << "fatal error - manager " << this->rank_ << " tried to access worker index " << n <<  " that doesn't exist" << endl;
            throw "fatal error - manager tried to access worker index that doesn't exist";
        }
    }
    catch (const string errorMessage) {
        cout << "ERROR: " << errorMessage << endl;
        assert(0);
    }
    return workerRanks_[n];
}

int MMC_Manager::worker_index_from_rank(int rank) {
    int index = -1;
    try {
        for (int i = 0; i < nWorkers_; ++i) {
            if (workerRanks_[i] == rank) {
                index = i;
            }
        }
        if (index == -1) {
            cout << "fatal error - manager " << this->rank_ << " tried to search for worker " << rank << " that doesn't exist" << endl;
            throw "fatal error - manager tried to search for worker that doesn't exist";
        }
    }
    catch (const string errorMessage) {
        cout << "ERROR: " << errorMessage << endl;
        assert(0);
    }
    return index;
}

/* Method to get the next rank to send work to.
 * The method call should be followed up by sending the
 * work to the worker rank.
 * Takes in no arguments, and returns an integer.
 */
int MMC_Manager::next_worker() {
    int startRank = workerRanks_[0], lastRank = workerRanks_[nWorkers_ - 1];
    for (int commRank = lastSentWorkRank_; commRank <= lastRank; ++commRank) {
        int flag;
        MPI_Iprobe(commRank, REQUESTING_TAG, MPI_COMM_WORLD, &flag, MPI_STATUS_IGNORE);
        if (flag) {
            //cout << this->rank_ << " has work for " << commRank << endl;
            lastSentWorkRank_ = commRank;
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
    try {
        if (!layoutFile.is_open()) {
            cout << "fatal error - no file opened" << endl;
            throw "fatal error - no file opened";
        }
    }
    catch(const string errorMessage) {
        cout << "ERROR: " << errorMessage << endl;
        assert(0);
    }
    
    vector<string> fileLayers;
    string tempLayerString;
    while (getline(layoutFile, tempLayerString)) {
        fileLayers.push_back(tempLayerString);
    }
    layoutFile.close();
    int numLayers = fileLayers.size();
    
    try {
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
    
    Layout_t layout(numLayers);
    for (int i = 0; i < numLayers; ++i) {
        int readIn;
        istringstream layeriss(fileLayers[i]);
        //int rank;
        //MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        while (layeriss >> readIn) {
            /*if (rank == 0) {
                cout << "push " << i << " - " << readIn << endl;
            }*/
            layout[i].push_back(readIn);
        }
    }
    
    try {
        for (unsigned int i = 1; i < layout.size(); ++i) {
            if (layout[i - 1].size() < layout[i].size()) {
                cout << "fatal error - layer " << i << " in layout has too many instances" << endl;
                throw "fatal error - layers have wrong number of instances";
            }
        }
        if (layout[numLayers - 1].size() != 1) {
            cout << "WARNING: top layer of cluster has more than one instance." << endl;
            cout << "Unexpected performance may result." << endl;
        }
    }
    catch(const string errorMessage) {
        cout << "ERROR: " << errorMessage << endl;
        assert(0);
    }

    return layout;
}

/* Reads the layout of the cluster from the
 * specified file, and arranges the data into a 
 * 2-dimensional vector which the user can treat as
 * an array.
 * Takes in a string, and returns a Layout_t variable.
 */
Layout_t read_cluster_layout_from_file(string& fileName) {
    ifstream layoutFile(fileName);
    try {
        if (!layoutFile.is_open()) {
            cout << "fatal error - no file opened" << endl;
            throw "fatal error - no file opened";
        }
    }
    catch(const string errorMessage) {
        cout << "ERROR: " << errorMessage << endl;
        assert(0);
    }
    
    vector<string> fileLayers;
    string tempLayerString;
    while (getline(layoutFile, tempLayerString)) {
        fileLayers.push_back(tempLayerString);
    }
    layoutFile.close();
    int numLayers = fileLayers.size();
    
    try {
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
    
    Layout_t layout(numLayers);
    for (int i = 0; i < numLayers; ++i) {
        int readIn;
        istringstream layeriss(fileLayers[i]);
        //int rank;
        //MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        while (layeriss >> readIn) {
            /*if (rank == 0) {
                cout << "push " << i << " - " << readIn << endl;
            }*/
            layout[i].push_back(readIn);
        }
    }
    
    try {
        for (unsigned int i = 1; i < layout.size(); ++i) {
            if (layout[i - 1].size() < layout[i].size()) {
                cout << "fatal error - layer " << i << " in layout has too many instances" << endl;
                throw "fatal error - layers have wrong number of instances";
            }
        }
        if (layout[numLayers - 1].size() != 1) {
            cout << "WARNING: top layer of cluster has more than one instance." << endl;
            cout << "Unexpected performance may result." << endl;
        }
    }
    catch(const string errorMessage) {
        cout << "ERROR: " << errorMessage << endl;
        assert(0);
    }

    return layout;
}
