#include <cstdlib>
#include <iostream>

#include "coproto/Socket/AsioSocket.h"
#include "cryptoTools/Common/Timer.h"
#include "cryptoTools/Common/block.h"

#include "ApplyLsh.h"
#include "Common/Parser.h"
#include "Common/Random.h"
#include "Common/Utils.h"
#include "FuzzyPcReceiver.h"
#include "FuzzyPcSender.h"
#include "argparse/argparse.hpp"

using namespace std;
using namespace coproto;
using namespace fuzzypc;
using namespace fuzzypc::common;

int main(int argc, char *argv[]) {
    argparse::ArgumentParser program("Receiver");
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
    program.add_argument("-lsh")
        .nargs(2)
        .default_value(vector<int>{0, 0})
        .scan<'i', int>();

    try {
        program.parse_args(argc, argv);
    } catch (const std::exception &err) {
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

    bool useLsh = false;
    if (numHashPerBand != 0 && numBands != 0) {
        useLsh = true;
    }
    size_t numThreads = 1;

    std::cout << "==== Start PPRL Receiver ====" << endl;
    std::cout << "- data : " << dataName << endl;

    string fileName, senderFileName, idName, fileNameLSH;
    if (dataName == "NCVR") {
        idName = "ID";
        fileName = dataFolderPath + "/NCVR/Streamlined_2014_1M.txt";
        senderFileName = dataFolderPath + "/NCVR/Streamlined_2017_1M.txt";
    } else if (dataName == "European") {
        idName = "PERSON_ID";
        fileName = dataFolderPath + "/European/cis.csv";
        senderFileName = dataFolderPath + "/European/census.csv";
    } else if (dataName == "DBLP-ACM") {
        idName = "id";
        fileName = dataFolderPath + "/DBLP-ACM/tableA.csv";
        senderFileName = dataFolderPath + "/DBLP-ACM/tableB.csv";
    }

    cout << "- read data ... ";
    size_t numRows1, numRows2;
    map<string, vector<string>> table1, table2, matches;
    if (dataName == "NCVR") {
        numRows1 = readTable(fileName, table1, 0, 0);
        numRows2 = readTable(senderFileName, table2, 0, 0);
    } else {
        numRows1 = readTable(fileName, table1);
        numRows2 = readTable(senderFileName, table2);
    }
    if (numRows1 > numRows && numRows != 0) {
        table1 = random(table1, numRows);
        numRows1 = numRows;
    };
    numRows = table1[idName].size();
    size_t numBins = ceil(1.3 * numRows);
    cout << "done" << endl;

    size_t numFeatures;
    vector<vector<string>> inputString;
    vector<vector<uint64_t>> inputUInt64;
    if (!useLsh) {
        cout << "- concatenate ... ";
        vector<vector<string>> features;
        if (dataName == "European") {
            for (size_t i = 0; i < table1["PERNAME1"].size(); i++) {
                table1["PERNAME1"][i] = soundex(table1["PERNAME1"][i]);
            }
            for (size_t i = 0; i < table1["PERNAME2"].size(); i++) {
                table1["PERNAME2"][i] = soundex(table1["PERNAME2"][i]);
            }
            features = {{"PERNAME1", "PERNAME2", "DOB_DAY", "DOB_MON", "SEX"},
                        {"PERNAME1", "PERNAME2", "DOB_DAY", "DOB_YEAR", "SEX"},
                        {"PERNAME1", "PERNAME2", "DOB_MON", "DOB_YEAR", "SEX"},
                        {"ENUMPC", "ENUMCAP", "DOB_DAY", "DOB_MON"},
                        {"ENUMPC", "ENUMCAP", "DOB_DAY", "DOB_YEAR"},
                        {"ENUMPC", "ENUMCAP", "DOB_MON", "DOB_YEAR"}};
            inputString = concatenate(table1, features, false, true);
        } else if (dataName == "NCVR") {
            features = {
                {"Name", "Addr", "Zip"}, {"Name", "Birth"}, {"Name", "Phone"}};
            inputString = concatenate(table1, features, false, false);
        }
        numFeatures = features.size();
        cout << "done" << endl;
    } else {
        cout << "- concatenate all ... ";
        vector<string> features, concated1;
        if (dataName == "DBLP-ACM") {
            features = {"title", "authors"};
            concated1 = concatenate(table1, features, true, false);
        } else if (dataName == "European") {
            features = {"PERNAME1", "PERNAME2", "DOB_DAY", "DOB_MON",
                        "DOB_YEAR", "SEX",      "ENUMPC",  "ENUMCAP"};
            concated1 = concatenate(table1, features, true, false);
        } else if (dataName == "NCVR") {
            features = {"Name", "Addr", "Zip", "Birth", "Phone"};
            concated1 = concatenate(table1, features, true, false);
        }
        cout << "done" << endl;

        cout << "- number of hash per band : " << numHashPerBand << endl;
        cout << "- number of band : " << numBands << endl;
        cout << "- apply LSH ... ";
        if (matcher == "jaccard") {
            inputUInt64 = applyJaccardLSH(concated1, numHashPerBand, numBands);
        } else if (matcher == "angular") {
            cout << "- apply LSH ... ";
            inputUInt64 = applyAngularLSH(concated1, numHashPerBand, numBands);
        }
        numFeatures = inputUInt64[0].size();
        cout << "done" << endl;
    }

    detail::init_global_asio_io_context();

    auto socket = coproto::asioConnect("localhost:12345", false);
    size_t numSenderRows = 0;
    sync_wait(socket.recv(numSenderRows));
    sync_wait(socket.send(std::move(numRows)));

    std::cout << "- num of rows (receiver) : " << numRows << endl;
    std::cout << "- num of rows (sender) : " << numSenderRows << endl;

    size_t itemByteSize = ((42 + ceil(log2(numRows * numSenderRows))) + 7) / 8;
    size_t payloadByteSize =
        ((40 + ceil(log2(numRows * numSenderRows))) + 7) / 8;

    cout << "- num of features after encoding : " << numFeatures << endl;

    vector<byte> payloads;
    osuCrypto::Timer timer;
    timer.setTimePoint("__Begin__");

    FuzzyPcReceiver receiver(numRows, numBins, itemByteSize, payloadByteSize,
                             numThreads, &timer);
    receiver.preprocess(numRows, numFeatures, payloadByteSize, socket);
    if (!useLsh) {
        payloads = receiver.run(inputString, socket);
    } else {
        payloads = receiver.run(inputUInt64, socket);
    }

    cout << "- receive sender's payload shares to check correctness" << endl;
    vector<byte> senderPayloadShares(numSenderRows * payloadByteSize);
    sync_wait(socket.recv(senderPayloadShares));
    sync_wait(socket.flush());
    socket.close();

    detail::destroy_global_asio_io_context();

    cout << "- compute true link ... ";

    if (numRows2 > numSenderRows && numSenderRows != 0) {
        table2 = random(table2, numSenderRows);
        numRows2 = numSenderRows;
    };
    if (dataName == "DBLP-ACM") {
        readTable(dataFolderPath + "/DBLP-ACM/matches.csv", matches);
    }

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

    vector<bool> matchedCheck(numRows);
    vector<pair<uint32_t, uint32_t>> successLink;
    for (auto &pair : trueLink) {
        matchedCheck[pair.first] = true;
        auto check = memcmp(
            senderPayloadShares.data() + pair.second * payloadByteSize,
            payloads.data() + pair.first * payloadByteSize, payloadByteSize);
        if (check == 0) {
            successLink.push_back(pair);
        }
    }

    vector<oc::block> receiverPayloadsInBlock;
    unordered_set<oc::block> senderPayloadsInBlock;
    for (size_t i = 0; i < numRows; i++) {
        oc::block tmp = oc::ZeroBlock;
        memcpy(&tmp, payloads.data() + i * payloadByteSize, payloadByteSize);
        receiverPayloadsInBlock.push_back(tmp);
    }
    for (size_t i = 0; i < numSenderRows; i++) {
        oc::block tmp = oc::ZeroBlock;
        memcpy(&tmp, senderPayloadShares.data() + i * payloadByteSize,
               payloadByteSize);
        senderPayloadsInBlock.insert(tmp);
    }

    auto realIntersectionSize = trueLink.size();
    uint32_t FP = 0;

    for (size_t i = 0; i < numRows; i++) {
        if (!matchedCheck[i]) {
            if (senderPayloadsInBlock.find(receiverPayloadsInBlock[i]) !=
                senderPayloadsInBlock.end()) {
                FP++;
            }
        }
    }

    uint32_t FN = realIntersectionSize - successLink.size();
    uint32_t TP = successLink.size();
    uint32_t TN = numRows - realIntersectionSize - FP;

    cout << "# Accuracy of Our PPRL protocol" << endl;

    cout << "- FN / TP / FP / TN : ";
    cout << FN << " / " << TP << " / " << FP << " / " << TN << endl;

    double acc = double(TP + TN) / double(TP + FP + FN + TN);
    double ppv = double(TP) / double(TP + FP);
    double recall = double(TP) / double(TP + FN);
    double f1 = double(2 * recall * ppv) / double(recall + ppv);

    cout << "- Acc : " << acc << endl;
    cout << "- Precision : " << ppv << endl;
    cout << "- Recall : " << recall << endl;
    cout << "- F1 : " << f1 << endl;

    std::cout << "==== DONE ====" << std::endl;

    return 0;
}