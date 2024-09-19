#include <iostream>

#include "gtest/gtest.h"

#include "coproto/Socket/AsioSocket.h"
#include "cryptoTools/Common/Timer.h"

#include "Common/Random.h"
#include "Protocols/CpsiReceiver.h"
#include "Protocols/CpsiSender.h"

using namespace std;
using namespace coproto;
using namespace fuzzypc::common;
using namespace fuzzypc::protocols;

class CPSITest : public ::testing::TestWithParam<pair<size_t, size_t>> {
   public:
    uint32_t keyByteSize = 12;
    uint32_t valueByteSize = 12;
};

TEST_P(CPSITest, CorrectnessTest) {
    auto param = GetParam();
    size_t numInputs = param.first, numCommonInputs = param.second;

    vector<byte> senderKeys(keyByteSize * numInputs);
    randomBytes(senderKeys.data(), keyByteSize * numInputs);
    vector<byte> senderValues(valueByteSize * numInputs);
    randomBytes(senderValues.data(), valueByteSize * numInputs);
    vector<byte> receiverKeys(keyByteSize * numInputs);
    randomBytes(receiverKeys.data(), keyByteSize * numInputs);
    for (size_t i = 0; i < keyByteSize * numCommonInputs; i++) {
        receiverKeys[i] = senderKeys[i];
    }

    oc::BitVector senderMembershipShares, receiverMembershipShares;
    vector<byte> senderPayloadShares, receiverPayloadShares;
    vector<uint64_t> itemToTableIdx;

    detail::init_global_asio_io_context();

    auto receiver = [&]() {
        auto socket = sync_wait(macoro::make_task(AsioAcceptor(
            "localhost:12345", detail::global_asio_io_context.value().mIoc)));
        CpsiReceiver cpsiReceiver(1);
        cpsiReceiver.preprocess(numInputs, socket);
        cpsiReceiver.run(receiverKeys, keyByteSize, valueByteSize,
                         receiverMembershipShares, receiverPayloadShares,
                         itemToTableIdx, socket);
        sync_wait(socket.flush());
        socket.close();
    };
    auto sender = [&]() {
        auto socket = sync_wait(macoro::make_task(AsioConnect(
            "localhost:12345", detail::global_asio_io_context.value().mIoc)));
        CPSISender cpsiSender(1);
        cpsiSender.preprocess(socket);
        cpsiSender.run(senderKeys, keyByteSize, senderValues, valueByteSize,
                       senderMembershipShares, senderPayloadShares, socket);
        sync_wait(socket.flush());
        socket.close();
    };

    thread t1(sender);
    thread t2(receiver);
    t1.join();
    t2.join();

    detail::destroy_global_asio_io_context();

    auto count = 0;
    EXPECT_EQ(senderMembershipShares.size(), receiverMembershipShares.size());
    for (size_t i = 0; i < numCommonInputs; i++) {
        auto idx = itemToTableIdx[i];
        auto membership =
            senderMembershipShares[idx] ^ receiverMembershipShares[idx];
        EXPECT_EQ(membership, true);
        vector<byte> xorResults(valueByteSize);
        for (size_t k = 0; k < valueByteSize; k++) {
            xorResults[k] = receiverPayloadShares[idx * valueByteSize + k] ^
                            senderPayloadShares[idx * valueByteSize + k];
        }
        auto check =
            memcmp(xorResults.data(), senderValues.data() + i * valueByteSize,
                   valueByteSize);
        EXPECT_EQ(check, 0);
    }
    for (size_t i = 0; i < senderMembershipShares.size(); i++) {
        if (senderMembershipShares[i] ^ receiverMembershipShares[i]) {
            count++;
        }
    }
    EXPECT_EQ(count, numCommonInputs);
}

INSTANTIATE_TEST_SUITE_P(CPSITest, CPSITest,
                         testing::Values(make_pair(10000, 100),
                                         make_pair(100000, 1000),
                                         make_pair(1000000, 10000)));