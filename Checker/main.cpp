#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <chrono>
#include <limits>

#include "Visualizer.h"

#include "ShortestPath.h"
#include "../Solver/PbReader.h"
#include "../Solver/SetCovering.pb.h"

using namespace std;
using namespace zqy;
using namespace pb;

int main(int argc, char *argv[]) {
    enum CheckerFlag {
        IoError = 0x0,
        FormatError = 0x1,
        NotCompleteCoverError = 0x2
    };

    string inputPath;
    string outputPath;

    if (argc > 1) {
        inputPath = argv[1];
    } else {
        cerr << "input path: " << flush;
        cin >> inputPath;
    }

    if (argc > 2) {
        outputPath = argv[2];
    } else {
        cerr << "output path: " << flush;
        cin >> outputPath;
    }

    pb::SetCovering::Input input;
    if (!load(inputPath, input)) { return ~CheckerFlag::IoError; }

    pb::SetCovering::Output output;
    ifstream ifs(outputPath);
    if (!ifs.is_open()) { return ~CheckerFlag::IoError; }
    string submission;
    getline(ifs, submission); // skip the first line.
    ostringstream oss;
    oss << ifs.rdbuf();
    jsonToProtobuf(oss.str(), output);
    cerr <<":" << output.chosen_columns().size() << endl;
    for (auto c = output.chosen_columns().begin(); c != output.chosen_columns().end(); ++c) {
        cerr << *c << " ";
    }cerr << endl;
    if (output.chosen_columns().size() <= 0) { cerr << ":" << output.chosen_columns().size() << endl;  return ~CheckerFlag::FormatError; }
   
    int rowNum = input.rownum();
    int columnNum = input.columnnum();

    int error = 0;
    // check objective.
    int SetNum ;
    double objScale = 1;

    vector<vector<int>>ColumnGather(columnNum);
    int conflict = rowNum;
    vector<bool> isSet(columnNum, false);
    vector < bool>isVisted(rowNum, false);

    
    for (auto c = output.chosen_columns().begin(); c != output.chosen_columns().end(); ++c) { isSet[*c] = true; }
    for (int j = 0; j != rowNum; ++j) {
        for (auto i = input.rowgathers(j).rowgather().begin(); i != input.rowgathers(j).rowgather().end(); ++i) {
            ColumnGather[*i].push_back(j);
            if (isSet[*i])isVisted[j] = true;
        }
    }
    for (int i = 0; i != rowNum; ++i) {
        if (isVisted[i] == false) { cout << i << endl; error |= CheckerFlag::NotCompleteCoverError; break; }
    }
   /* for (int j = 0; j != rowNum; ++j) {
        for (auto i = input.rowgathers(j).rowgather().begin(); i != input.rowgathers(j).rowgather().end(); ++i) {
            ColumnGather[*i].push_back(j);
        }
    }
    for (auto j = 0; j != columnNum; ++j) {
        if (isSet[j] == false)continue;
        for (auto i = ColumnGather[j].begin(); i != ColumnGather[j].end(); ++i) {
            if (isVisted[*i] == false) {
                isVisted[*i] = true;
                --conflict;
            }
        }
    }*/
    /*cout << conflict << endl;
    if (conflict > 0) error |= CheckerFlag::NotCompleteCoverError;*/
    SetNum = output.chosen_columns().size(); 
    int returnCode = (error == 0) ? SetNum : ~error;
    cout << "||******************|| " << ((error == 0) ? (SetNum) : returnCode) << " ||**************||" << endl;
    return returnCode;
}