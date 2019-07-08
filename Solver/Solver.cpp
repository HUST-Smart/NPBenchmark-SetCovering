#include "Solver.h"

#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <mutex>
#include <cmath>


using namespace std;


namespace zqy {

#pragma region Solver::Cli
int Solver::Cli::run(int argc, char * argv[]) {//argv是运行文件名，argc是文件名所含字符数
    Log(LogSwitch::Zqy::Cli) << "parse command line arguments." << endl;
    Set<String> switchSet;
    Map<String, char*> optionMap({ // use string as key to compare string contents instead of pointers.
        {InstancePathOption(), nullptr },
        {InitSolutionPathOption(), nullptr},
        { SolutionPathOption(), nullptr },
        { RandSeedOption(), nullptr },
        { TimeoutOption(), nullptr },
        { MaxIterOption(), nullptr },
        { JobNumOption(), nullptr },
        { RunIdOption(), nullptr },
        { EnvironmentPathOption(), nullptr },
        { ConfigPathOption(), nullptr },
        { LogPathOption(), nullptr }
        });

    for (int i = 1; i < argc; ++i) { // skip executable name.
        auto mapIter = optionMap.find(argv[i]);
        if (mapIter != optionMap.end()) { // option argument.
            mapIter->second = argv[++i];
        } else { // switch argument.
            switchSet.insert(argv[i]);
        }
    }

    Log(LogSwitch::Zqy::Cli) << "execute commands." << endl;
    if (switchSet.find(HelpSwitch()) != switchSet.end()) {
        cout << HelpInfo() << endl;
    }

    if (switchSet.find(AuthorNameSwitch()) != switchSet.end()) {
        cout << AuthorName() << endl;
    }

    Solver::Environment env;
    env.load(optionMap);
    if (env.instPath.empty() || env.slnPath.empty() || env.initPath.empty()) { return -1; }

    Solver::Configuration cfg;
    cfg.load(env.cfgPath);

    Log(LogSwitch::Zqy::Input) << "load instance " << env.instPath << " (seed=" << env.randSeed << ")." << endl;
    Problem::Input input;
    Problem::Output initoutput;
    if (!input.load(env.instPath)) { return -1; }
    initoutput.load(env.initPath);
    //if (!initoutput.load(env.initPath)) { return -1; }//初始解load
    //Log(LogSwitch::Szx::Output)<<"load init solution "<<env.initPath<< endl;
    Solver solver(input, initoutput, env, cfg);

    solver.solve();
    pb::Submission submission;
    submission.set_thread(to_string(env.jobNum));
    submission.set_instance(env.friendlyInstName());
    submission.set_duration(to_string(solver.timer.elapsedSeconds()) + "s");

    solver.output.save(env.slnPath, submission);
    #if ZQY_DEBUG

    solver.output.save(env.solutionPathWithTime(), submission);
    solver.record();
    #endif 

    return 0;
}
#pragma endregion Solver::Cli

#pragma region Solver::Environment
void Solver::Environment::load(const Map<String, char*> &optionMap) {
    char *str;

    str = optionMap.at(Cli::EnvironmentPathOption());
    if (str != nullptr) { loadWithoutCalibrate(str); }

    str = optionMap.at(Cli::InstancePathOption());
    if (str != nullptr) { instPath = str; }

    str = optionMap.at(Cli::InitSolutionPathOption());
    if (str != nullptr) { initPath = str; }

    str = optionMap.at(Cli::SolutionPathOption());
    if (str != nullptr) { slnPath = str; }

    str = optionMap.at(Cli::RandSeedOption());
    if (str != nullptr) { randSeed = atoi(str); }

    str = optionMap.at(Cli::TimeoutOption());
    if (str != nullptr) { msTimeout = static_cast<Duration>(atof(str) * Timer::MillisecondsPerSecond); }

    str = optionMap.at(Cli::MaxIterOption());
    if (str != nullptr) { maxIter = atoi(str); }

    str = optionMap.at(Cli::JobNumOption());
    if (str != nullptr) { jobNum = atoi(str); }

    str = optionMap.at(Cli::RunIdOption());
    if (str != nullptr) { rid = str; }

    str = optionMap.at(Cli::ConfigPathOption());
    if (str != nullptr) { cfgPath = str; }

    str = optionMap.at(Cli::LogPathOption());
    if (str != nullptr) { logPath = str; }

    calibrate();
}

void Solver::Environment::load(const String &filePath) {
    loadWithoutCalibrate(filePath);
    calibrate();
}

void Solver::Environment::loadWithoutCalibrate(const String &filePath) {
    // EXTEND[szx][8]: load environment from file.
    // EXTEND[szx][8]: check file existence first.
}

void Solver::Environment::save(const String &filePath) const {
    // EXTEND[szx][8]: save environment to file.
}
void Solver::Environment::calibrate() {
    // adjust thread number.
    int threadNum = thread::hardware_concurrency();
    if ((jobNum <= 0) || (jobNum > threadNum)) { jobNum = threadNum; }

    // adjust timeout.
    msTimeout -= Environment::SaveSolutionTimeInMillisecond;
}
#pragma endregion Solver::Environment

#pragma region Solver::Configuration
void Solver::Configuration::load(const String &filePath) {
    // EXTEND[szx][5]: load configuration from file.
    // EXTEND[szx][8]: check file existence first.
}

void Solver::Configuration::save(const String &filePath) const {
    // EXTEND[szx][5]: save configuration to file.
}
#pragma endregion Solver::Configuration

#pragma region Solver
bool Solver::solve() {
    init();

    int workerNum = (max)(1, env.jobNum / cfg.threadNumPerWorker);
    cfg.threadNumPerWorker = env.jobNum / workerNum;
    List<Solution> solutions(workerNum, Solution(this));
    List<bool> success(workerNum);

    Log(LogSwitch::Zqy::Framework) << "launch " << workerNum << " workers." << endl;
    List<thread> threadList;
    threadList.reserve(workerNum);


    for (int i = 0; i < workerNum; ++i) {
        // TODO[szx][2]: as *this is captured by ref, the solver should support concurrency itself, i.e., data members should be read-only or independent for each worker.
        // OPTIMIZE[szx][3]: add a list to specify a series of algorithm to be used by each threads in sequence.

        threadList.emplace_back([&, i]() { success[i] = optimize(solutions[i], i); });
        
    }
    for (int i = 0; i < workerNum; ++i) { threadList.at(i).join(); }

    Log(LogSwitch::Zqy::Framework) << "collect best result among all workers." << endl;
    int bestIndex = -1;
    Length bestValue = Problem::MaxDistance;
    for (int i = 0; i < workerNum; ++i) {
        if (!success[i]) { continue; }
        Log(LogSwitch::Zqy::Framework) << "worker " << i << " got " << solutions[i].coverRadius << endl;
        if (solutions[i].coverRadius >= bestValue) { continue; }
        bestIndex = i;
        bestValue = solutions[i].coverRadius;
    }

    env.rid = to_string(bestIndex);
    if (bestIndex < 0) { return false; }
    output = solutions[bestIndex];
    initoutput = solutions[bestIndex];
    return true;
}

void Solver::record() const {
    #if ZQY_DEBUG
    int generation = 0;

    ostringstream log;

    System::MemoryUsage mu = System::peakMemoryUsage();

    Length obj = output.chosen_columns().size();
    Length checkerObj = -1;
    bool feasible = check(checkerObj);
    // record basic information.
    log << env.friendlyLocalTime() << ","
        << env.rid << ","
        << env.instPath << ","
        << feasible << ",";
        auto oldPrecision = log.precision();
        log << ((obj - checkerObj)) << ",";
        log.precision(oldPrecision);
    log << timer.elapsedSeconds() << ","
        << mu.physicalMemory << "," << mu.virtualMemory << ","
        << env.randSeed << ","
        << cfg.toBriefStr() << ","
        << generation << "," << iteration << ",";
    // record solution vector.
    // EXTEND[szx][2]: save solution in log.
    log << endl;

    // append all text atomically.
    static mutex logFileMutex;
    lock_guard<mutex> logFileGuard(logFileMutex);

    ofstream logFile(env.logPath, ios::app);
    logFile.seekp(0, ios::end);
    if (logFile.tellp() <= 0) {
        logFile << "Time,ID,Instance,Feasible,ObjMatch,Duration,PhysMem,VirtMem,RandSeed,Config,Generation,Iteration,Solution" << endl;
    }
    logFile << log.str();
    logFile.close();
    #endif // ZQY_DEBUG
}
bool Solver::check(Length &checkerObj) const {
    #if ZQY_DEBUG
    enum CheckerFlag {
        IoError = 0x0,
        FormatError = 0x1,
        NotCompleteCoverError = 0x2
    };
    checkerObj = 0;
    checkerObj = System::exec("Checker.exe " + env.instPath + " " + env.solutionPathWithTime());
    if (checkerObj > 0) { return true; }
    checkerObj = ~checkerObj;
    if (checkerObj == CheckerFlag::IoError) { Log(LogSwitch::Checker) << "IoError." << endl; }
    if (checkerObj & CheckerFlag::FormatError) { Log(LogSwitch::Checker) << "FormatError." << endl; }
    if (checkerObj & CheckerFlag::NotCompleteCoverError) { Log(LogSwitch::Checker) << "NotCompleteCoverError." << endl; }
    return false;
    #else
    checkerObj = 0;
    return true;
    #endif // ZQY_DEBUG
}


void Solver::init() {

    ID rowNum = input.rownum();
    ID columnNum = input.columnnum(); 
    Data.RowGather.resize(rowNum);
    Data.ColumnGather.resize(columnNum);
    for (int i = 0; i < rowNum; i++) {
        for (auto j = input.rowgathers(i).rowgather().begin(); j != input.rowgathers(i).rowgather().end(); j++) {
            Data.RowGather[i].push_back(*j);
            Data.ColumnGather[*j].push_back(i);
        }
    }   
}


bool Solver::optimize(Solution &sln,  ID workerId) {
    Log(LogSwitch::Zqy::Framework) << "worker " << workerId << " starts." << endl;
    bool status = true;
    ID rowNum = input.rownum();
    ID columnNum = input.columnnum();
// TODO[0]: replace the following random assignment with your own algorithm.
    vector<bool>covering(rowNum,false);
    int tempC = rand() % columnNum;
    int unCovering_num = rowNum;

    sln.add_chosen_columns(tempC);
    for (auto i = Data.ColumnGather[tempC].begin(); i != Data.ColumnGather[tempC].end(); ++i) {
        if (covering[*i] == false) {
            covering[*i] = true;
            unCovering_num--;
        }
    }
    while (unCovering_num > 0) {
        vector<int>unCovering;
        for (int j = 0; j != rowNum; ++j) {
            if (covering[j] == false) {
                unCovering.push_back(j);
            }
        }
        int tempR = rand() % unCovering.size();
       
        int ColumnToRowNum=0;
        for (auto i = Data.RowGather[unCovering[tempR]].begin(); i != Data.RowGather[unCovering[tempR]].end(); ++i) {
            if (ColumnToRowNum < Data.ColumnGather[*i].size()) {
                ColumnToRowNum = Data.ColumnGather[*i].size();
                tempC = *i;
            }
        }
        sln.add_chosen_columns(tempC);
        for (auto i = Data.ColumnGather[tempC].begin(); i != Data.ColumnGather[tempC].end(); ++i) {
            if (covering[*i] == false) {
                covering[*i] = true;
                unCovering_num--;
            }
        }
    }
    for (auto i = sln.chosen_columns().begin(); i != sln.chosen_columns().end(); ++i) {
        cout << *i << ",";
    }
    cout <<endl<<"conflict:"<< unCovering_num<< endl;

    Log(LogSwitch::Zqy::Framework) << "worker " << workerId << " ends." << endl;
    return status;
}


#pragma endregion Solver
}