/* 본 프로그램에 대한 저작권을 포함한 지적재산권은 삼성SDS(주)에 있으며,
삼성SDS(주)가 명시적으로 허용하지 않은 사용, 복사, 변경, 제3자에의 공개,
배포는 엄격히 금지되며, 삼성SDS(주)의 지적재산권 침해에 해당됩니다.
Copyright (c) 2023 Samsung SDS Co., Ltd. All Rights Reserved. Confidential.
All information including the intellectual and technical concepts contained
herein is, and remains the property of Samsung SDS Co. Ltd. Unauthorized use,
dissemination, or reproduction of this material is strictly forbidden unless
prior written permission is obtained from Samsung SDS Co. Ltd.
*/

#include <algorithm>
#include <iostream>
#include <string>
#include <thread>
#include <unordered_set>

#include "ApplyLsh.h"
#include "Common/Parser.h"
#include "Common/Utils.h"
#include "argparse/argparse.hpp"

using namespace std;
using namespace fuzzypc::common;

int main(int argc, char* argv[]) {
    argparse::ArgumentParser program("Experiment1");
    program.add_argument("-n", "--name")
        .required()
        .help("specify the name of dataset.");
    program.add_argument("-r", "--rows")
        .help("specify the number of rows.")
        .default_value(0)
        .scan<'i', int>();
    program.add_argument("-m", "--matcher")
        .required()
        .help("specify the name of matcher.");
    program.add_argument("-t", "--threshold")
        .default_value(0.0)
        .help("specify the float value of threshold.")
        .scan<'g', double>();
    program.add_argument("-lsh")
        .nargs(2)
        .default_value(vector<int>{0, 0})
        .scan<'i', int>();

    try {
        program.parse_args(argc, argv);
    } catch (const std::exception& err) {
        cerr << err.what() << endl;
        cerr << program;
        exit(1);
    }

    string file{__FILE__};
    string directory{file.substr(0, file.rfind("/"))};
    string dataFolderPath = directory + "/data";

    string dataName = program.get("-n");
    size_t numRows = program.get<int>("-r");
    string matcher = program.get("-m");
    auto lsh = program.get<vector<int>>("-lsh");
    size_t numHashPerBand = lsh[0];
    size_t numBands = lsh[1];
    double threshold = program.get<double>("-t");

    if (numHashPerBand != 0 && numBands != 0) {
        matcher = matcher + "-lsh";
    }

    cout << "#######################################" << endl;
    cout << "# Experiment for Table 2 in Our Paper #" << endl;
    cout << "#######################################" << endl;

    cout << "- dataFolderPath : " << dataFolderPath << endl;
    cout << "- dataName : " << dataName << endl;
    cout << "- matcher : " << matcher << endl;

    string filePath1, filePath2, filePath3;
    if (dataName == "DBLP-ACM") {
        filePath1 = dataFolderPath + "/DBLP-ACM/tableA.csv";
        filePath2 = dataFolderPath + "/DBLP-ACM/tableB.csv";
        filePath3 = dataFolderPath + "/DBLP-ACM/matches.csv";
    } else if (dataName == "European") {
        filePath1 = dataFolderPath + "/European/census.csv";
        filePath2 = dataFolderPath + "/European/cis.csv";
    } else if (dataName == "NCVR") {
        filePath1 = dataFolderPath + "/NCVR/Streamlined_2014_1M.txt";
        filePath2 = dataFolderPath + "/NCVR/Streamlined_2017_1M.txt";
    }

    cout << "- filePath1 : " << filePath1 << endl;
    cout << "- filePath2 : " << filePath2 << endl;

    cout << "- read data ... ";
    size_t numRows1, numRows2;
    map<string, vector<string>> table1, table2, matches;
    if (dataName == "NCVR") {
        numRows1 = readTable(filePath1, table1, 0, 0);
        numRows2 = readTable(filePath2, table2, 0, 0);
    } else {
        numRows1 = readTable(filePath1, table1);
        numRows2 = readTable(filePath2, table2);
    }
    if (numRows1 > numRows && numRows != 0) {
        table1 = random(table1, numRows);
        numRows1 = numRows;
    };
    if (numRows2 > numRows && numRows != 0) {
        table2 = random(table2, numRows);
        numRows2 = numRows;
    };
    if (dataName == "DBLP-ACM") {
        readTable(filePath3, matches);
    }
    cout << "done" << endl;
    cout << "\t* size of table1 : " << numRows1 << " X " << table1.size()
         << endl;
    cout << "\t* size of table2 : " << numRows2 << " X " << table2.size()
         << endl;

    cout << "- compute true link ... ";
    unordered_map<size_t, size_t> trueLink;
    string id;
    if (dataName == "European") {
        id = "PERSON_ID";
    } else if (dataName == "NCVR") {
        id = "ID";
    }
    if (dataName == "DBLP-ACM") {
        unordered_map<string, size_t> mapFromIdToIndex1, mapFromIdToIndex2;
        for (size_t i = 0; i < table1["id"].size(); i++) {
            mapFromIdToIndex1[table1["id"][i]] = i;
        }
        for (size_t i = 0; i < table2["id"].size(); i++) {
            mapFromIdToIndex2[table2["id"][i]] = i;
        }
        for (size_t i = 0; i < matches["idDBLP"].size(); i++) {
            trueLink[mapFromIdToIndex1[matches["idDBLP"][i]]] =
                mapFromIdToIndex2[matches["idACM"][i]];
        }
    } else if (dataName == "European" || dataName == "NCVR") {
        unordered_map<string, size_t> mapFromIdToIndex2;
        for (size_t i = 0; i < table2[id].size(); i++) {
            mapFromIdToIndex2[table2[id][i]] = i;
        }
        for (size_t i = 0; i < table1[id].size(); i++) {
            if (mapFromIdToIndex2.find(table1[id][i]) !=
                mapFromIdToIndex2.end()) {
                trueLink[i] = mapFromIdToIndex2[table1[id][i]];
            }
        }
    }
    cout << "done" << endl;
    cout << "\t* number of true links : " << trueLink.size() << endl;

    if (matcher == "equality") {
        cout << "- concatenate ... ";
        vector<vector<string>> features, concated1, concated2;
        if (dataName == "European") {
            for (size_t i = 0; i < table1["PERNAME1"].size(); i++) {
                table1["PERNAME1"][i] = soundex(table1["PERNAME1"][i]);
            }
            for (size_t i = 0; i < table1["PERNAME2"].size(); i++) {
                table1["PERNAME2"][i] = soundex(table1["PERNAME2"][i]);
            }
            for (size_t i = 0; i < table2["PERNAME1"].size(); i++) {
                table2["PERNAME1"][i] = soundex(table2["PERNAME1"][i]);
            }
            for (size_t i = 0; i < table2["PERNAME2"].size(); i++) {
                table2["PERNAME2"][i] = soundex(table2["PERNAME2"][i]);
            }
            features = {{"PERNAME1", "PERNAME2", "DOB_DAY", "DOB_MON", "SEX"},
                        {"PERNAME1", "PERNAME2", "DOB_DAY", "DOB_YEAR", "SEX"},
                        {"PERNAME1", "PERNAME2", "DOB_MON", "DOB_YEAR", "SEX"},
                        {"ENUMPC", "ENUMCAP", "DOB_DAY", "DOB_MON"},
                        {"ENUMPC", "ENUMCAP", "DOB_DAY", "DOB_YEAR"},
                        {"ENUMPC", "ENUMCAP", "DOB_MON", "DOB_YEAR"}};
            concated1 = concatenate(table1, features, false, true);
            concated2 = concatenate(table2, features, false, true);
        } else if (dataName == "NCVR") {
            features = {
                {"Name", "Addr", "Zip"}, {"Name", "Birth"}, {"Name", "Phone"}};
            concated1 = concatenate(table1, features, false, false);
            concated2 = concatenate(table2, features, false, false);
        }
        size_t numFeatures = features.size();
        cout << "done" << endl;

        cout << "- pair-wise comparison ... ";
        unordered_map<size_t, size_t> link = equality(concated1, concated2);
        cout << "done" << endl;

        cout << "- accuracy" << endl;
        accuracy(trueLink, link, concated1.size());

        return 0;
    }

    cout << "- concatenate all ... ";
    vector<string> features, concated1, concated2;
    if (dataName == "DBLP-ACM") {
        features = {"title", "authors"};
        concated1 = concatenate(table1, features, true, false);
        concated2 = concatenate(table2, features, true, false);
    } else if (dataName == "European") {
        features = {"PERNAME1", "PERNAME2", "DOB_DAY", "DOB_MON",
                    "DOB_YEAR", "SEX",      "ENUMPC",  "ENUMCAP"};
        concated1 = concatenate(table1, features, true, false);
        concated2 = concatenate(table2, features, true, false);
    } else if (dataName == "NCVR") {
        features = {"Name", "Addr", "Zip", "Birth", "Phone"};
        concated1 = concatenate(table1, features, true, false);
        concated2 = concatenate(table2, features, true, false);
    }
    cout << "done" << endl;

    unordered_map<size_t, size_t> link;
    if (matcher == "jaccard-lsh") {
        cout << "- number of hash per band : " << numHashPerBand << endl;
        cout << "- number of band : " << numBands << endl;
        cout << "- apply LSH ... ";
        auto applied1 = applyJaccardLSH(concated1, numHashPerBand, numBands);
        auto applied2 = applyJaccardLSH(concated2, numHashPerBand, numBands);
        cout << "done" << endl;

        cout << "- find link ... ";
        link = threshold1(applied1, applied2);
        cout << "done" << endl;

        cout << "- accuracy" << endl;
        accuracy(trueLink, link, concated1.size());
        return 0;
    } else if (matcher == "angular-lsh") {
        cout << "- apply LSH ... ";
        auto applied1 = applyAngularLSH(concated1, numHashPerBand, numBands);
        auto applied2 = applyAngularLSH(concated2, numHashPerBand, numBands);
        cout << "done" << endl;

        cout << "- find link ... ";
        link = threshold1(applied1, applied2);
        cout << "done" << endl;

        cout << "- accuracy" << endl;
        accuracy(trueLink, link, concated1.size());
        return 0;
    }
    if (matcher == "jaccard") {
        cout << "- convert to set ... ";
        size_t shingle = 2;
        set<string> s;
        vector<vector<string>> transformed1(concated1.size());
        for (size_t i = 0; i < concated1.size(); i++) {
            if (concated1[i] != "") {
                s.clear();
                for (size_t j = 0; j < concated1[i].length() - shingle + 1;
                     j++) {
                    s.insert(concated1[i].substr(j, shingle));
                }
                transformed1[i].resize(s.size());
                copy(s.begin(), s.end(), transformed1[i].begin());
            }
        }
        vector<vector<string>> transformed2(concated2.size());
        for (size_t i = 0; i < concated2.size(); i++) {
            if (concated2[i] != "") {
                s.clear();
                for (size_t j = 0; j < concated2[i].length() - shingle + 1;
                     j++) {
                    s.insert(concated2[i].substr(j, shingle));
                }
                transformed2[i].resize(s.size());
                copy(s.begin(), s.end(), transformed2[i].begin());
            }
        }
        cout << "done" << endl;

        cout << "- pair-wise comparison ... ";
        vector<thread> worker;
        size_t numThreads = 8;
        size_t numRowsInThreads =
            (transformed1.size() + numThreads - 1) / numThreads;
        auto job = [&](int i) {
            size_t start = i * numRowsInThreads;
            size_t end = start + numRowsInThreads;
            end = min(transformed1.size(), end);
            for (size_t j = start; j < end; j++) {
                double max = 0.0;
                size_t maxIndex;
                for (size_t k = 0; k < transformed2.size(); k++) {
                    if (transformed1[j].size() != 0 &&
                        transformed2[k].size() != 0) {
                        double similarity = JaccardLsh::jaccord(
                            transformed1[j], transformed2[k]);
                        if (similarity > max) {
                            max = similarity;
                            maxIndex = k;
                        }
                    }
                }
                if (max > threshold) {
                    link[j] = maxIndex;
                }
            }
        };
        for (size_t i = 0; i < numThreads; i++) {
            worker.push_back(thread(job, i));
        }
        for (size_t i = 0; i < numThreads; i++) {
            worker[i].join();
        }
        cout << " done" << endl;
    } else if (matcher == "angular") {
        cout << "- convert to vector ... ";
        set<string> keys;
        string candidates = "abcdefghijklmnopqrstuvwxyz0123456789";
        string qgram;
        qgram.resize(2);
        for (size_t i = 0; i < candidates.size(); i++) {
            qgram[0] = candidates[i];
            for (size_t j = 0; j < candidates.size(); j++) {
                qgram[1] = candidates[j];
                keys.insert(qgram);
            }
        }
        CountVectorizer cv;
        cv.setKeys(keys);
        vector<SparseVector> transformed1(concated1.size());
        vector<SparseVector> transformed2(concated2.size());
        for (size_t i = 0; i < concated1.size(); i++) {
            transformed1[i] = cv.transformToSparse(concated1[i]);
        }
        for (size_t i = 0; i < concated2.size(); i++) {
            transformed2[i] = cv.transformToSparse(concated2[i]);
        }
        cout << "done" << endl;

        cout << "- pair-wise comparison ... ";
        vector<thread> worker;
        size_t numThreads = 6;
        size_t numRowsInThreads =
            (transformed1.size() + numThreads - 1) / numThreads;
        auto job = [&](int i) {
            size_t start = i * numRowsInThreads;
            size_t end = start + numRowsInThreads;
            end = min(transformed1.size(), end);
            for (size_t j = start; j < end; j++) {
                double max = 0.0;
                size_t maxIndex;
                for (size_t k = 0; k < transformed2.size(); k++) {
                    if (concated1[j] != "" && concated2[k] != "") {
                        double similarity = AngularLsh::angular(
                            transformed1[j], transformed2[k]);
                        if (similarity > max) {
                            max = similarity;
                            maxIndex = k;
                        }
                    }
                }
                if (max > threshold) {
                    link[j] = maxIndex;
                }
            }
        };
        for (size_t i = 0; i < numThreads; i++) {
            worker.push_back(thread(job, i));
        }
        for (size_t i = 0; i < numThreads; i++) {
            worker[i].join();
        }
        cout << " done" << endl;
    }

    cout << "- accuracy" << endl;
    accuracy(trueLink, link, concated1.size());

    return 0;
}
