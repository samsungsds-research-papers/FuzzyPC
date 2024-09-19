#include "gtest/gtest.h"

#include <thread>

#include "cryptoTools/Crypto/AES.h"

#include "Common/Random.h"
#include "Protocols/EqualityCheck.h"
#include "coproto/Socket/AsioSocket.h"

using namespace std;
using namespace coproto;
using namespace fuzzypc::common;
using namespace fuzzypc::protocols;

class EqualityCheckTest
    : public ::testing::TestWithParam<pair<size_t, size_t>> {
   public:
    size_t inputByteSize = 8;
    size_t inputBitSize = 64;
};

TEST_P(EqualityCheckTest, CorrectnessTest) {
    auto param = GetParam();
    size_t numInputs = param.first, numCommonInputs = param.second;

    vector<byte> inputsSender(inputByteSize * numInputs);
    vector<byte> inputsReceiver(inputByteSize * numInputs);
    randomBytes(inputsSender.data(), inputByteSize * numInputs);
    randomBytes(inputsReceiver.data(), inputByteSize * numInputs);
    for (size_t i = 0; i < inputByteSize * numCommonInputs; i++) {
        inputsSender[i] = inputsReceiver[i];
    }

    detail::init_global_asio_io_context();

    oc::BitVector resultSender, resultReceiver;
    auto sender = [&]() {
        auto socket = sync_wait(macoro::make_task(AsioAcceptor(
            "localhost:12345", detail::global_asio_io_context.value().mIoc)));
        EqualityCheck equalityCheck(true, 1);
        size_t numOTs = equalityCheck.getNumOTs(numInputs, inputBitSize);
        equalityCheck.sendRandomOTs(numOTs + 10, socket);
        resultSender = equalityCheck.run(inputsSender, inputBitSize, socket);
        sync_wait(socket.flush());
        socket.close();
    };
    auto receiver = [&]() {
        auto socket = sync_wait(macoro::make_task(AsioConnect(
            "localhost:12345", detail::global_asio_io_context.value().mIoc)));
        EqualityCheck equalityCheck(false, 1);
        size_t numOTs = equalityCheck.getNumOTs(numInputs, inputBitSize);
        equalityCheck.recvRandomOTs(numOTs + 10, socket);
        resultReceiver =
            equalityCheck.run(inputsReceiver, inputBitSize, socket);
        sync_wait(socket.flush());
        socket.close();
    };

    thread t1(sender);
    thread t2(receiver);
    t1.join();
    t2.join();

    detail::destroy_global_asio_io_context();

    oc::BitVector result = resultSender ^ resultReceiver;
    for (size_t i = 0; i < numInputs; i++) {
        if (i < numCommonInputs) {
            EXPECT_EQ(result[i], 1);
        } else {
            EXPECT_EQ(result[i], 0);
        }
    }
}

INSTANTIATE_TEST_SUITE_P(EqualityCheckTest, EqualityCheckTest,
                         testing::Values(make_pair(10000, 100),
                                         make_pair(100000, 1000),
                                         make_pair(1000000, 10000)));
