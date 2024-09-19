#include <cstdlib>
#include <iostream>
#include <thread>

#include "coproto/Socket/AsioSocket.h"
#include "cryptoTools/Common/Timer.h"
#include "cryptoTools/Common/block.h"

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
    size_t numRowsSender = stoi(argv[1]);
    size_t numRowsReceiver = stoi(argv[2]);
    size_t numFeatures = stoi(argv[3]);
    size_t numThreads = stoi(argv[4]);

    cout << "######################################" << endl;
    cout << "# Start Benchmark (w/ Random Inputs) #" << endl;
    cout << "######################################" << endl;

    cout << "- numRowsSender : " << numRowsSender << endl;
    cout << "- numRowsReceiver : " << numRowsReceiver << endl;
    cout << "- numFeatures : " << numFeatures << endl;
    cout << "- numThreads : " << numThreads << endl;

    detail::init_global_asio_io_context();

    auto sender = [&]() {
        auto socket = sync_wait(macoro::make_task(AsioAcceptor(
            "localhost:12345", detail::global_asio_io_context.value().mIoc)));

        uint64_t numBins = ceil(1.3 * numRowsReceiver);
        size_t itemByteSize =
            ((42 + ceil(log2(numRowsReceiver * numRowsSender))) + 7) / 8;
        size_t payloadByteSize =
            ((40 + ceil(log2(numRowsReceiver * numRowsSender))) + 7) / 8;

        vector<vector<string>> inputs;
        inputs.resize(numRowsSender);
        for (size_t i = 0; i < numRowsSender; i++) {
            inputs[i].resize(numFeatures);
            for (size_t j = 0; j < numFeatures; j++) {
                inputs[i][j] = common::randomString(16);
            }
        }

        osuCrypto::Timer timer;
        timer.setTimePoint("__Begin__");

        vector<byte> dummy(itemByteSize);
        memcpy(dummy.data(), &oc::AllOneBlock, itemByteSize);
        FuzzyPcSender sender(numRowsSender, numBins, itemByteSize,
                             payloadByteSize, numThreads, &timer);
        sender.preprocess(numRowsSender, numFeatures, payloadByteSize, socket);

        cout << "- communication cost (preprocess) : "
             << (socket.bytesSent() + socket.bytesReceived()) /
                    (1024.0 * 1024.0)
             << "MB" << endl;

        sender.run(inputs, socket);

        cout << "- timming result of our fuzzy private computation protocol : "
             << endl;
        sender.printTime();
        cout << "- communication cost (total) : " << endl;
        cout << "\t* sender -> receiver : "
             << socket.bytesSent() / (1024.0 * 1024.0) << "MB" << endl;
        cout << "\t* receiver -> sender : "
             << socket.bytesReceived() / (1024.0 * 1024.0) << "MB" << endl;
        cout << "\t* total : "
             << (socket.bytesSent() + socket.bytesReceived()) /
                    (1024.0 * 1024.0)
             << "MB" << endl;

        sync_wait(socket.flush());
        socket.close();
    };

    auto receiver = [&]() {
        auto socket = sync_wait(macoro::make_task(AsioConnect(
            "localhost:12345", detail::global_asio_io_context.value().mIoc)));

        uint64_t numBins = ceil(1.3 * numRowsReceiver);

        size_t itemByteSize =
            ((42 + ceil(log2(numRowsReceiver * numRowsSender))) + 7) / 8;
        size_t payloadByteSize =
            ((40 + ceil(log2(numRowsReceiver * numRowsSender))) + 7) / 8;

        vector<vector<string>> inputs;
        inputs.resize(numRowsReceiver);
        for (size_t i = 0; i < numRowsReceiver; i++) {
            inputs[i].resize(numFeatures);
            for (size_t j = 0; j < numFeatures; j++) {
                inputs[i][j] = common::randomString(16);
            }
        }

        vector<byte> ids;
        osuCrypto::Timer timer;
        timer.setTimePoint("__Begin__");

        vector<byte> dummy(itemByteSize);
        memcpy(dummy.data(), &oc::CCBlock, itemByteSize);
        FuzzyPcReceiver receiver(numRowsReceiver, numBins, itemByteSize,
                                 payloadByteSize, numThreads, &timer);
        receiver.preprocess(numRowsReceiver, numFeatures, payloadByteSize,
                            socket);
        ids = receiver.run(inputs, socket);

        sync_wait(socket.flush());
        socket.close();
    };

    thread t1(sender);
    thread t2(receiver);
    t1.join();
    t2.join();

    detail::destroy_global_asio_io_context();

    return 0;
}