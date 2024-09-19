#include "gtest/gtest.h"

#include <iostream>

#include "coproto/Socket/AsioSocket.h"

#include "Common/Random.h"
#include "DataStructures/IntegerPermutation.h"
#include "Protocols/PnsReceiver.h"
#include "Protocols/PnsSender.h"

using namespace std;
using namespace coproto;
using namespace fuzzypc::common;
using namespace fuzzypc::protocols;

using namespace std;
using namespace coproto;
using namespace fuzzypc::common;
using namespace fuzzypc::protocols;

class PnsTest : public ::testing::TestWithParam<size_t> {
   public:
    uint32_t inputByteSize = 12;
};

TEST_P(PnsTest, CorrectnessTest) {
    size_t numInputs = GetParam();

    vector<byte> receiverInput(inputByteSize * numInputs);
    randomBytes(receiverInput.data(), inputByteSize * numInputs);

    vector<byte> senderOutput, receiverOutput;

    fuzzypc::structures::IntegerPermutation permutation(0, numInputs - 1);

    srand(unsigned(time(0)));
    permutation.randomShuffle();

    detail::init_global_asio_io_context();

    auto sender = [&]() {
        auto socket = sync_wait(macoro::make_task(AsioAcceptor(
            "localhost:12347", detail::global_asio_io_context.value().mIoc)));
        PnsSender pnsSender(1);
        pnsSender.configure(numInputs, inputByteSize);
        pnsSender.recvRandomOTs(socket);
        senderOutput = pnsSender.run(permutation, socket);
        sync_wait(socket.flush());
        socket.close();
    };

    auto receiver = [&]() {
        auto socket = sync_wait(macoro::make_task(AsioConnect(
            "localhost:12347", detail::global_asio_io_context.value().mIoc)));
        PnsReceiver pnsReceiver(1);
        pnsReceiver.configure(numInputs, inputByteSize);
        pnsReceiver.sendRandomOTs(socket);
        receiverOutput = pnsReceiver.run(receiverInput, socket);
        sync_wait(socket.flush());
        socket.close();
    };

    thread t1(sender);
    thread t2(receiver);
    t1.join();
    t2.join();

    detail::destroy_global_asio_io_context();

    for (size_t i = 0; i < numInputs; i++) {
        size_t j = permutation.get(i);
        for (size_t k = 0; k < inputByteSize; k++) {
            EXPECT_EQ(receiverInput[i * inputByteSize + k],
                      senderOutput[j * inputByteSize + k] ^
                          receiverOutput[j * inputByteSize + k]);
        }
    }
}

INSTANTIATE_TEST_SUITE_P(PnsTest, PnsTest,
                         testing::Values(10000, 100000, 1000000));