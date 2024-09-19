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

    string fileName, idName;
    if (dataName == "NCVR") {
        idName = "ID";
        fileName = dataFolderPath + "/NCVR/Streamlined_2017_1M.txt";
    } else if (dataName == "European") {
        idName = "PERSON_ID";
        fileName = dataFolderPath + "/European/census.csv";
    } else if (dataName == "DBLP") {
        idName = "id";
        fileName = dataFolderPath + "/tableB.csv";
    }

    cout << "- read data ... ";
    size_t numRows1;
    map<string, vector<string>> table1;
    if (dataName == "NCVR") {
        numRows1 = readTable(fileName, table1, 0, 0);
    } else {
        numRows1 = readTable(fileName, table1);
    }
    if (numRows1 > numRows && numRows != 0) {
        table1 = random(table1, numRows);
        numRows1 = numRows;
    };
    numRows = table1[idName].size();
    cout << "done" << endl;

    detail::init_global_asio_io_context();

    auto socket = coproto::asioConnect("localhost:12345", true);
    size_t numReceiverRows;
    sync_wait(socket.send(std::move(numRows)));
    sync_wait(socket.recv(numReceiverRows));
    uint64_t numBins = ceil(1.3 * numReceiverRows);

    size_t itemByteSize =
        ((42 + ceil(log2(numReceiverRows * numRows))) + 7) / 8;
    size_t payloadByteSize =
        ((40 + ceil(log2(numReceiverRows * numRows))) + 7) / 8;

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

    vector<byte> payloads;
    osuCrypto::Timer timer;
    timer.setTimePoint("__Begin__");

    FuzzyPcSender sender(numRows, numBins, itemByteSize, payloadByteSize,
                         numThreads, &timer);
    sender.preprocess(numRows, numFeatures, payloadByteSize, socket);
    if (!useLsh) {
        sender.run(inputString, socket);
    } else {
        sender.run(inputUInt64, socket);
    }

    payloads = sender.getPayloadsInByte();

    std::cout << "# Timming result of Our PPRL protocol" << endl;
    sender.printTime();
    std::cout << "# Communication cost" << endl;
    std::cout << "- S -> R : " << socket.bytesSent() / (1024.0 * 1024.0) << "MB"
              << std::endl;
    std::cout << "- R -> R : " << socket.bytesReceived() / (1024.0 * 1024.0)
              << "MB" << std::endl;
    std::cout << "- Total : "
              << (socket.bytesSent() + socket.bytesReceived()) /
                     (1024.0 * 1024.0)
              << "MB" << std::endl;

    cout << "- send payload shares to check correctness" << endl;
    sync_wait(socket.send(std::move(payloads)));
    sync_wait(socket.flush());
    socket.close();

    detail::destroy_global_asio_io_context();

    std::cout << "==== DONE ====" << std::endl;

    return 0;
}