#include "Simulator.h"


using namespace std;
using namespace zqy;


int main() {
    //Simulator::initDefaultEnvironment();

    Simulator sim;
   // while(1)
       // sim.debug();
    sim.benchmark(1);
    //sim.parallelBenchmark(10);
    //sim.parallelBenchmark(2);
    //sim.generateInstance();
    //for (int i = 1; i <= 10; ++i) { sim.convertScpInstance("Instance/scp/scp4/scp4", i); }
    //for (int i = 1; i <= 10; ++i) { sim.convertScpInstance("Instance/scp/scp5/scp5", i); }
    //for (int i = 1; i <= 5; ++i) { sim.convertScpInstance("Instance/scp/scp6/scp6", i); }
   // for (int i = 1; i <= 5; ++i) { sim.convertScpInstance("Instance/scp/scpa/scpa", i); }
    //for (int i = 1; i <= 5; ++i) { sim.convertScpInstance("Instance/scp/scpb/scpb", i); }
    return 0;
}
