#include <iostream>

#include "gtest/gtest.h"

#include "coproto/Socket/AsioSocket.h"
#include "cryptoTools/Common/Timer.h"
#include "cryptoTools/Crypto/PRNG.h"

#include "libOTe/Base/BaseOT.h"
#include "libOTe/TwoChooseOne/Silent/SilentOtExtReceiver.h"
#include "libOTe/TwoChooseOne/Silent/SilentOtExtSender.h"

#include "Common/Random.h"
#include "Protocols/SecretSharedOt.h"

using namespace std;
using namespace coproto;
using namespace fuzzypc::common;
using namespace fuzzypc::protocols;

class SecretSharedOtTest : public ::testing::TestWithParam<size_t> {
   public:
    uint32_t msgByteSize = 12;
    oc::PRNG prng{oc::ZeroBlock};
};

TEST_P(SecretSharedOtTest, CorrectnessTest) {
    auto numInputs = GetParam();

    array<vector<byte>, 2> senderMsgSharesInByte;
    array<vector<byte>, 2> receiverMsgSharesInByte;
    array<vector<byte>, 2> msgInByte;
    for (size_t c = 0; c < 2; c++) {
        senderMsgSharesInByte[c].resize(msgByteSize * numInputs);
        randomBytes(senderMsgSharesInByte[c].data(), msgByteSize * numInputs);
        receiverMsgSharesInByte[c].resize(msgByteSize * numInputs);
        randomBytes(receiverMsgSharesInByte[c].data(), msgByteSize * numInputs);
        msgInByte[c].resize(msgByteSize * numInputs);
        for (size_t i = 0; i < msgByteSize * numInputs; i++) {
            msgInByte[c][i] =
                senderMsgSharesInByte[c][i] ^ receiverMsgSharesInByte[c][i];
        }
    }

    oc::BitVector senderChoiceShares(numInputs);
    senderChoiceShares.randomize(prng);
    oc::BitVector receiverChoiceShares(numInputs);
    receiverChoiceShares.randomize(prng);

    vector<byte> senderOutputSharesInByte;
    vector<byte> receiverOutputSharesInByte;

    detail::init_global_asio_io_context();

    auto sender = [&]() {
        auto socket = sync_wait(macoro::make_task(AsioAcceptor(
            "localhost:12345", detail::global_asio_io_context.value().mIoc)));
        SecretSharedOT sender(true, 1);
        sender.preprocess(numInputs, socket);
        senderOutputSharesInByte = sender.run(
            senderChoiceShares, senderMsgSharesInByte, msgByteSize, socket);
        sync_wait(socket.flush());
        socket.close();
    };

    auto receiver = [&]() {
        auto socket = sync_wait(macoro::make_task(AsioConnect(
            "localhost:12345", detail::global_asio_io_context.value().mIoc)));
        SecretSharedOT receiver(false, 1);
        receiver.preprocess(numInputs, socket);
        receiverOutputSharesInByte = receiver.run(
            receiverChoiceShares, receiverMsgSharesInByte, msgByteSize, socket);
        sync_wait(socket.flush());
        socket.close();
    };

    thread t1(sender);
    thread t2(receiver);
    t1.join();
    t2.join();

    detail::destroy_global_asio_io_context();

    auto choice = senderChoiceShares ^ receiverChoiceShares;
    vector<byte> xorCheck(msgByteSize * numInputs);
    for (size_t i = 0; i < msgByteSize * numInputs; i++) {
        xorCheck[i] =
            senderOutputSharesInByte[i] ^ receiverOutputSharesInByte[i];
    }
    for (size_t i = 0; i < numInputs; i++) {
        size_t idx = i * msgByteSize;
        auto check = memcmp(xorCheck.data() + idx,
                            msgInByte[choice[i]].data() + idx, msgByteSize);
        EXPECT_EQ(check, 0);
    }
}

INSTANTIATE_TEST_SUITE_P(SecretSharedOtTest, SecretSharedOtTest,
                         testing::Values(100000, 1000000));