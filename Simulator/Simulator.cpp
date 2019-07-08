#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <algorithm>
#include <random>

#include <cstring>

#include "Simulator.h"
#include "ThreadPool.h"


using namespace std;


namespace zqy {

    // EXTEND[szx][5]: read it from InstanceList.txt.
static const vector<String> instList({
     "scp41.r200c1000",
     "scp42.r200c1000",
     "scp43.r200c1000",
     "scp44.r200c1000",
     "scp45.r200c1000",
     "scp46.r200c1000",
     "scp47.r200c1000",
     "scp48.r200c1000",
     "scp49.r200c1000",
     "scp410.r200c1000",
     "scp51.r200c1000",
     "scp52.r200c2000",
     "scp53.r200c2000",
     "scp54.r200c2000",
     "scp55.r200c2000",
     "scp56.r200c2000",
     "scp57.r200c2000",
     "scp58.r200c2000",
     "scp59.r200c2000",
     "scp510.r200c2000",
     "scp61.r200c1000",
     "scp62.r200c1000",
     "scp63.r200c1000",
     "scp64.r200c1000",
     "scp65.r200c1000",
     "scpa1.r300c3000",
     "scpa2.r300c3000",
     "scpa3.r300c3000",
     "scpa4.r300c3000",
     "scpa5.r300c3000",
     "scpb1.r300c3000",
     "scpb2.r300c3000",
     "scpb3.r300c3000",
     "scpb4.r300c3000",
     "scpb5.r300c3000",
    });

void Simulator::initDefaultEnvironment() {
    Solver::Environment env;
    env.save(Env::DefaultEnvPath());

    Solver::Configuration cfg;
    cfg.save(Env::DefaultCfgPath());
}

void Simulator::run(const Task &task) {
    String instanceName(task.instSet + task.instId + ".json");
    String solutionName(task.instSet + task.instId + ".json");
    String initSolutionName(task.instSet + task.instId + ".json");

    char argBuf[Cmd::MaxArgNum][Cmd::MaxArgLen];
    char *argv[Cmd::MaxArgNum];
    for (int i = 0; i < Cmd::MaxArgNum; ++i) { argv[i] = argBuf[i]; }
    strcpy(argv[ArgIndex::ExeName], ProgramName().c_str());

    int argc = ArgIndex::ArgStart;

    strcpy(argv[argc++], Cmd::InstancePathOption().c_str());
    strcpy(argv[argc++], (InstanceDir() + instanceName).c_str());

    strcpy(argv[argc++], Cmd::InitSolutionPathOption().c_str());
    strcpy(argv[argc++], (InitSolutionDir() + initSolutionName).c_str());

    System::makeSureDirExist(SolutionDir());
    strcpy(argv[argc++], Cmd::SolutionPathOption().c_str());
    strcpy(argv[argc++], (SolutionDir() + solutionName).c_str());

    if (!task.randSeed.empty()) {
        strcpy(argv[argc++], Cmd::RandSeedOption().c_str());
        strcpy(argv[argc++], task.randSeed.c_str());
    }

    if (!task.timeout.empty()) {
        strcpy(argv[argc++], Cmd::TimeoutOption().c_str());
        strcpy(argv[argc++], task.timeout.c_str());
    }

    if (!task.maxIter.empty()) {
        strcpy(argv[argc++], Cmd::MaxIterOption().c_str());
        strcpy(argv[argc++], task.maxIter.c_str());
    }

    if (!task.jobNum.empty()) {
        strcpy(argv[argc++], Cmd::JobNumOption().c_str());
        strcpy(argv[argc++], task.jobNum.c_str());
    }

    if (!task.runId.empty()) {
        strcpy(argv[argc++], Cmd::RunIdOption().c_str());
        strcpy(argv[argc++], task.runId.c_str());
    }

    if (!task.cfgPath.empty()) {
        strcpy(argv[argc++], Cmd::ConfigPathOption().c_str());
        strcpy(argv[argc++], task.cfgPath.c_str());
    }

    if (!task.logPath.empty()) {
        strcpy(argv[argc++], Cmd::LogPathOption().c_str());
        strcpy(argv[argc++], task.logPath.c_str());
    }

    Cmd::run(argc, argv);
}

void Simulator::run(const String &envPath) {
    char argBuf[Cmd::MaxArgNum][Cmd::MaxArgLen];
    char *argv[Cmd::MaxArgNum];
    for (int i = 0; i < Cmd::MaxArgNum; ++i) { argv[i] = argBuf[i]; }
    strcpy(argv[ArgIndex::ExeName], ProgramName().c_str());

    int argc = ArgIndex::ArgStart;

    strcpy(argv[argc++], Cmd::EnvironmentPathOption().c_str());
    strcpy(argv[argc++], envPath.c_str());

    Cmd::run(argc, argv);
}

void Simulator::debug() {
    Task task;
    task.instSet = "";
    task.instId = "scp41.n200s1000" ;
    //task.randSeed = "1500972793";
    task.randSeed = to_string(Random::generateSeed());
    //task.randSeed = to_string(RandSeed::generate());
    task.timeout = "180";
    //task.maxIter = "1000000000";
    task.jobNum = "1";
    task.cfgPath = Env::DefaultCfgPath();
    task.logPath = Env::DefaultLogPath();
    task.runId = "0";

    run(task);
}

void Simulator::benchmark(int repeat) {
    Task task;
    task.instSet = "";
    //task.timeout = "180";
    //task.maxIter = "1000000000";
    task.timeout = "3600";
    //task.maxIter = "1000000000";
    task.jobNum = "1";
    task.cfgPath = Env::DefaultCfgPath();
    task.logPath = Env::DefaultLogPath();

    random_device rd;
    mt19937 rgen(rd());
    for (int i = 0; i < repeat; ++i) {
        //shuffle(instList.begin(), instList.end(), rgen);
        for (auto inst = instList.begin(); inst != instList.end(); ++inst) {
            task.instId = *inst;
            task.randSeed = to_string(Random::generateSeed());
            task.runId = to_string(i);
            run(task);
        }
    }
}

void Simulator::parallelBenchmark(int repeat) {
    Task task;
    task.instSet = "";
    //task.timeout = "180";
    //task.maxIter = "1000000000";
    task.timeout = "3600";
    //task.maxIter = "1000000000";
    task.jobNum = "1";
    task.cfgPath = Env::DefaultCfgPath();
    task.logPath = Env::DefaultLogPath();

    ThreadPool<> tp(2);

    random_device rd;
    mt19937 rgen(rd());
    for (int i = 0; i < repeat; ++i) {
        //shuffle(instList.begin(), instList.end(), rgen);
        for (auto inst = instList.begin(); inst != instList.end(); ++inst) {
            task.instId = *inst;
            task.randSeed = to_string(Random::generateSeed());
            task.runId = to_string(i);
            tp.push([=]() { run(task); });
            this_thread::sleep_for(1s);
        }
    }
}

void Simulator::generateInstance(const InstanceTrait &trait) {
    Random rand;

    Problem::Input input;

    // EXTEND[szx][5]: generate random instances.

    ostringstream path;
    path << InstanceDir() << "rand.n" << input.rownum()
        << "s" << input.columnnum() << ".json";
    save(path.str(), input);
}


void Simulator::convertScpInstance(const String &scpPath, int index) {
    Log(Log::Info) << "converting " << scpPath << index << endl;

    ifstream ifs(scpPath + to_string(index) + ".txt");
    if (!ifs)cout << "error";
    int rowNum, columnNum;
    ifs >> rowNum >> columnNum;
    Arr2D<int> edgeIndices(rowNum, rowNum, -1);
    Problem::Input input;
    input.set_columnnum(columnNum);
    input.set_rownum(rowNum);
    for (int e = 0; e < columnNum; ++e) {
        int tosetnum;
        ifs >> tosetnum;
       /// input.add_settonodenum(tosetnum);
    }
    for (int i = 0; i != input.rownum(); ++i) {
        int tosetnum;
        ifs >> tosetnum;
        int setid;
        auto &nodeToset(*input.add_rowgathers());
        nodeToset.set_id(i);
        for (int s = 0; s != tosetnum; ++s) {  
            //auto &setCover(*nodeToset.add_nodetoset());
            ifs >> setid;
            nodeToset.add_rowgather(setid-1);
        }
    }
    
    
    ostringstream path;
    path << InstanceDir() << "scp6" << index << ".r" << input.rownum()
        << "c" << input.columnnum()  << ".json";
    save(path.str(), input);
}

}
