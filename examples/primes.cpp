#include "mmc.h"

#include <cmath>
#include <iostream>
#include <mpi.h>
#include <omp.h>
#include <vector>

using namespace std;

const int maxCheck = 16000000;
const int smallestGap = 500000;

bool isPrime(int);
int primeCalc(int[]);
void recvWork(int[], int);
void sendWork(int[], int, MPI_Request*);
void receiveSendResults(MMC_Manager&);
int receiveResults(MMC_Manager& manager);
int fillGaps(int*, MMC_Manager&);
bool incrementWorkUnit(int[], int, int);
void worker_function(MMC_Machine&);
void middle_manager_function(MMC_Machine&);
void top_manager_function(MMC_Machine&);

int main(void) {
    MPI_Init(NULL, NULL);
    Layout_t layout = read_cluster_layout_from_file();
    MPI_Barrier(MPI_COMM_WORLD);
    
    MMC_Machine machine(layout);
    
    if (machine.role() == "worker") {
        worker_function(machine);
    }
    else {
        if (machine.is_top()) {
            top_manager_function(machine);
        }
        else {
            middle_manager_function(machine);
        }
    }
    
    MPI_Finalize();
    return 0;
}

bool isPrime(int n) {
    int sqRoot = sqrt(n);
    for (int i = 2; i <= sqRoot; ++i) {
        if (n % i == 0) {
            return false;
        }
    }
    return true;
}

int primeCalc(int workInfo[]) {
    int total = 0;
    for (int i = workInfo[0]; i <= workInfo[1]; i += 2) {
        if (isPrime(i)) {
            ++total;
        }
    }
    return total;
}

void recvWork(int workInfo[], int commRank) {
    MPI_Recv(workInfo, 2, MPI_INT, commRank, WORK_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
}

void sendWork(int workInfo[], int commRank, MPI_Request* request) {
    MPI_Isend(workInfo, 2, MPI_INT, commRank, WORK_TAG, MPI_COMM_WORLD, request);
}

void receiveSendResults(MMC_Manager& manager) {
    int total = receiveResults(manager);
    MPI_Send(&total, 1, MPI_INT, manager.manager_rank(), RESULTS_TAG, MPI_COMM_WORLD);
}

int receiveResults(MMC_Manager& manager) {
    int total = 0, recvTemp;
    int start = manager.first_worker(), end = manager.last_worker();
    for (int i = start; i <= end; ++i) {
        MPI_Recv(&recvTemp, 1, MPI_INT, i, RESULTS_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        total += recvTemp;
    }
    return total;
}

int fillGaps(int* gaps, MMC_Manager& manager) {
    int numLayers = (manager.layout()).size();
    gaps[0] = 0;
    gaps[1] = smallestGap;
    for (int i = 2; i < numLayers; ++i) {
        int sizeLower = (manager.layout())[i - 1].size();
        int sizeCurr = (manager.layout())[i].size();
        int multiplier = sizeLower / sizeCurr;
        gaps[i] = gaps[i - 1] * multiplier;
    }
    return gaps[manager.level()];
}

bool incrementWorkUnit(int workInfo[], int gapToUse, int workEnd) {
    if (workInfo[1] == workEnd) {
        return true;
    }
    else {
        workInfo[0] += gapToUse;
        workInfo[1] = workInfo[0] + gapToUse - 1;
        if (workInfo[1] > workEnd) {
            workInfo[1] = workEnd;
        }
    }
    return false;
}

void worker_function(MMC_Machine& machine) {
    MMC_Worker worker;
    worker = machine;
    int managerRank = worker.manager_rank();
    int workInfo[2];
    int primeCount = 0;
    
    bool hasMoreWork = worker.get_more_work();
    while (hasMoreWork) {
        recvWork(workInfo, managerRank);
        primeCount += primeCalc(workInfo);
        hasMoreWork = worker.get_more_work();
    }
    MPI_Send(&primeCount, 1, MPI_INT, managerRank, RESULTS_TAG, MPI_COMM_WORLD);
}

void middle_manager_function(MMC_Machine& machine) {
    MMC_Manager manager;
    manager = machine;
    int* gaps = new int[(manager.layout()).size()];
    int gapToUse = fillGaps(gaps, manager);
    
    int workEnd;
    int workInfo[2];
    int managerRank = manager.manager_rank();
    MPI_Request requestDummy;
    bool hasMoreWork = manager.get_more_work();
    while (hasMoreWork) {
        recvWork(workInfo, managerRank);
        workEnd = workInfo[1];
        workInfo[1] = workInfo[0] + gapToUse - 1;
        
        bool isDone = false;
        while (!isDone) {
            int nextRank = manager.next_worker();
            
            sendWork(workInfo, nextRank, &requestDummy);
            
            isDone = incrementWorkUnit(workInfo, gapToUse, workEnd);
        }
        hasMoreWork = manager.get_more_work();
    }
    delete[] (gaps);
    manager.clear_worker_queue();
    receiveSendResults(manager);
}

void top_manager_function(MMC_Machine& machine) {
    double startTime = omp_get_wtime();
    MMC_Manager manager;
    manager = machine;
    int* gaps = new int[(manager.layout()).size()];
    int gapToUse = fillGaps(gaps, manager);
    
    int workStart = 3, workEnd = maxCheck;
    int workInfo[2] = {workStart, workStart + (gapToUse -1)};
    MPI_Request requestDummy;
    bool isDone = false;
    while (!isDone) {
        int nextRank = manager.next_worker();
        
        sendWork(workInfo, nextRank, &requestDummy);
        
        isDone = incrementWorkUnit(workInfo, gapToUse, workEnd);
    }
    manager.clear_worker_queue();
    
    int totalPrimes = receiveResults(manager);
    ++totalPrimes;
    
    double endTime = omp_get_wtime();
    cout << totalPrimes << " primes found in " << endTime - startTime << " seconds." << endl;
}
