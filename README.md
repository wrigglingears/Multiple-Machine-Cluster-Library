# Multiple-Machine-Cluster-Library
It is a collection of C++ classes that help in organizing hierarchical cluster organisation and dynamic work allocation.

I started programming a 'pi cluster' with a combination of MPI and OpenMP, which yielded satisfactory results but was at times difficult to analyze and maintain. It was also difficult to properly relate the individual MPI instances in a way that was meaningful.

Therefore I created this Multiple Machine Cluster Library, or MMC for short. This library defines a partlcular cluster programming paradigm in a C++ environment.

Each MPI instance has its own MMC_Machine object, which holds all the information about that instance. From there, the machine can either do work or assign work, based on the desired organisation of the cluster.
Machines which are assigned to do work are MMC_Workers, while those assigned to give work are MMC_Managers. MMC_Managers could also report to other MMC_Managers, if the layout of the cluster has multiple levels of hierarchy. 

This library allows for dynamic scheduling of work between machines through class methods that ask for, and give more work. Here is a skeleton code to demonstrate this feature:

    void worker_function(Machine& machine) { //layout stores the cluster layout plan
    
        MMC_Worker worker; 
        worker = machine; //faster initialization using values already stored in machine
        
        //setup code here
        
        int manager = machine.get_manager_rank();
        bool hasMoreWork = worker.get_more_work(); //returns whether the manager has more work to allocate
        while (hasMoreWork) {
        //get work unit from manager
      
        //do work based on workInfo
      
        hasMoreWork = worker.get_more_work();
        }
    
        //return results to manager;
    }


    void manager_function(Machine& machine) {
    
        MMC_Manager manager;
        manager = machine;
    
        //setup code here
    
        while (work units are not all given out) {
            int nextRank = manager.get_next_worker(); //returns the next worker in line for work
            
            //send work unit to nextRank
            
            //set up for next work unit
        }
        manager.clear_worker_queue(); //informs all workers that there's no more work
    
        //receive results from individual workers
    }

Specific examples can be found in the examples folder.
