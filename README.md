# Multiple-Machine-Cluster-Library
It is a collection of C++ classes that help in organizing hierarchical cluster organisation and dynamic work allocation.

I started programming a compute cluster with a combination of MPI and OpenMP, which yielded satisfactory results but was at times difficult to analyze and debug. It was also difficult to properly relate the individual MPI instances in a way that was meaningful.

Therefore I created this Multiple Machine Cluster Library, or MMC for short. This library defines a partlcular cluster programming paradigm in a C++ environment.

Each MPI instance has its own MMC_Machine object, which holds all the information about that instance. From there, the machine can either do work or assign work, based on the desired organisation of the cluster.
Machines which are assigned to do work are MMC_Workers, while those assigned to give work are MMC_Managers.

This library allows for dynamic scheduling of work between machines through class methods that ask for, and give more work. A skeleton example to demonstrate the library looks something like this:


void worker_function(Layout_t& layout) { //layout stores the cluster layout plan
    
    MMC_Worker worker(layout);
    
    //setup code here
    
    while (hasMoreWork) {
      //get work unit 
      
      //do work based on workInfo
      
      hasMoreWork = worker.get_more_work();
    }
    
    //return results to worker.get_reporting_rank();
}


void manager_function(Layout& layout) {
    
    MMC_Manager manager(layout);
    
    //setup code here
    
    while (work units are not all given out) {
        int nextRank = manager.get_send_work_rank();
        //send work unit to nextRank
        //set up for next work unit
    }
    manager.clear_worker_queue();
    
    //receive results from individual workers
}
