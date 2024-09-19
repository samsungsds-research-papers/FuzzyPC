/* 본 프로그램에 대한 저작권을 포함한 지적재산권은 삼성SDS(주)에 있으며,
삼성SDS(주)가 명시적으로 허용하지 않은 사용, 복사, 변경, 제3자에의 공개,
배포는 엄격히 금지되며, 삼성SDS(주)의 지적재산권 침해에 해당됩니다.
Copyright (c) 2023 Samsung SDS Co., Ltd. All Rights Reserved. Confidential.
All information including the intellectual and technical concepts contained
herein is, and remains the property of Samsung SDS Co. Ltd. Unauthorized use,
dissemination, or reproduction of this material is strictly forbidden unless
prior written permission is obtained from Samsung SDS Co. Ltd.
*/

#include "SecretSharedOt.h"

#include "libOTe/TwoChooseOne/Silent/SilentOtExtReceiver.h"
#include "libOTe/TwoChooseOne/Silent/SilentOtExtSender.h"

#include "Common/Random.h"

using namespace std;
using namespace coproto;
using namespace osuCrypto;

namespace fuzzypc::protocols {
vector<byte> SecretSharedOT::run(const BitVector &choiceShares,
                                 const array<vector<byte>, 2> &msgShares,
                                 size_t msgByteSize, Socket &chl) {
    uint64_t numOTs = choiceShares.size();
    if (mRemainRandomOt < numOTs) {
        cout << Color::Blue
             << "WARNING: random OTs ran out in SecretSharedOT, so perform "
                "additional random OTs. We note that generate random OTs at "
                "once is more faster."
             << Color::Default << endl;
        preprocess(numOTs - mRemainRandomOt, chl);
        mRemainRandomOt = numOTs;
    }

    // Convert
    vector<array<block, 2>> msgSharesAsBlock(numOTs);
    for (size_t i = 0; i < numOTs; i++) {
        for (size_t j = 0; j < 2; j++) {
            memcpy(&msgSharesAsBlock[i][j],
                   msgShares[j].data() + i * msgByteSize, msgByteSize);
        }
    }

    // Run partial OTs two times
    vector<block> share1, share2;
    if (mIsSender) {
        recvPartialOTs(choiceShares, msgByteSize, share1, chl);
        sendPartialOTs(choiceShares, msgSharesAsBlock, msgByteSize, share2,
                       chl);
    } else {
        sendPartialOTs(choiceShares, msgSharesAsBlock, msgByteSize, share1,
                       chl);
        recvPartialOTs(choiceShares, msgByteSize, share2, chl);
    }

    // Compute xor and convert to bytes
    vector<byte> outputSharesInByte(numOTs * msgByteSize);
    for (size_t i = 0; i < numOTs; i++) {
        block tmp = share2[i] ^ share1[i];
        memcpy(outputSharesInByte.data() + i * msgByteSize, &tmp, msgByteSize);
    }

    // Update
    mRandomOtIdx += numOTs;
    mRemainRandomOt -= numOTs;

    if (mTimer != nullptr) {
        mTimer->setTimePoint("secretsharedot.run");
    }

    return outputSharesInByte;
}

void SecretSharedOT::preprocess(size_t numOTs, Socket &chl) {
    if (mIsSender) {
        sendRandomOTs(numOTs, chl);
        recvRandomOTs(numOTs, chl);
    } else {
        recvRandomOTs(numOTs, chl);
        sendRandomOTs(numOTs, chl);
    }
    mRemainRandomOt += numOTs;

    if (mTimer != nullptr) {
        mTimer->setTimePoint("secretsharedot.preprocess");
    }
};

void SecretSharedOT::sendPartialOTs(const BitVector &choiceShares,
                                    const vector<array<block, 2>> &msgs,
                                    size_t msgByteSize, vector<block> &outputs,
                                    Socket &chl) {
    uint64_t numOTs = msgs.size();
    BitVector correction(numOTs);
    sync_wait(chl.recv(correction));

    // Pick random masking
    outputs.resize(numOTs);
    mPrng.get(outputs.data(), outputs.size());

    correction ^= choiceShares;  // randomchoice ^ c0 ^ c1

    vector<array<block, 2>> ctxtsInBlock(numOTs);
    for (size_t i = 0; i < numOTs; i++) {
        ctxtsInBlock[i][choiceShares[i]] =
            mRandomTwoMessages[i + mRandomOtIdx][correction[i]] ^ outputs[i] ^
            msgs[i][0];
        ctxtsInBlock[i][!choiceShares[i]] =
            mRandomTwoMessages[i + mRandomOtIdx][!correction[i]] ^ outputs[i] ^
            msgs[i][1];
    }

    array<vector<byte>, 2> ctxtsInByte;
    ctxtsInByte[0].resize(numOTs * msgByteSize);
    ctxtsInByte[1].resize(numOTs * msgByteSize);
    for (size_t i = 0; i < numOTs; i++) {
        memcpy(ctxtsInByte[0].data() + i * msgByteSize, &ctxtsInBlock[i][0],
               msgByteSize);
        memcpy(ctxtsInByte[1].data() + i * msgByteSize, &ctxtsInBlock[i][1],
               msgByteSize);
    }

    sync_wait(when_all_ready(chl.send(move(ctxtsInByte[0])),
                             chl.send(move(ctxtsInByte[1]))));
    // sync_wait(chl.send(move(ctxtsInByte[0])));
    // sync_wait(chl.send(move(ctxtsInByte[1])));
}

void SecretSharedOT::recvPartialOTs(const BitVector &choiceShares,
                                    size_t msgByteSize, vector<block> &outputs,
                                    Socket &socket) {
    uint64_t numOTs = choiceShares.size();
    BitVector correction;
    correction.append(mRandomChoices.data(), numOTs, mRandomOtIdx);
    correction ^= choiceShares;
    sync_wait(socket.send(move(correction)));

    array<vector<byte>, 2> ctxtsInByte;
    ctxtsInByte[0].resize(numOTs * msgByteSize);
    ctxtsInByte[1].resize(numOTs * msgByteSize);
    sync_wait(when_all_ready(socket.recv(ctxtsInByte[0]),
                             socket.recv(ctxtsInByte[1])));
    // sync_wait(socket.recv(ctxtsInByte[0]));
    // sync_wait(socket.recv(ctxtsInByte[1]));

    vector<array<block, 2>> ctxtsInBlock(numOTs);
    for (size_t i = 0; i < numOTs; i++) {
        memcpy(&ctxtsInBlock[i][0], ctxtsInByte[0].data() + i * msgByteSize,
               msgByteSize);
        memcpy(&ctxtsInBlock[i][1], ctxtsInByte[1].data() + i * msgByteSize,
               msgByteSize);
    }

    outputs.resize(numOTs);
    for (size_t i = 0; i < numOTs; i++) {
        outputs[i] = ctxtsInBlock[i][choiceShares[i]] ^
                     mRandomMessages[i + mRandomOtIdx];
    }
}

void SecretSharedOT::sendRandomOTs(size_t numOTs, Socket &socket) {
    vector<array<block, 2>> randomMsgs(numOTs);

    // Random OTs
    SilentOtExtSender sender_;
    sender_.mMultType = MultType::ExConv7x24;
    sender_.configure(numOTs, 2, mNumThreads);
    sync_wait(sender_.genSilentBaseOts(mPrng, socket, true));
    sync_wait(sender_.silentSend(randomMsgs, mPrng, socket));

    // Check Timing Result
    if (mTimer != nullptr) {
        mTimer->setTimePoint("SecretSharedOT.sendrandomots.silentsend");
    }

    mRandomTwoMessages.insert(mRandomTwoMessages.end(), randomMsgs.begin(),
                              randomMsgs.end());
}

void SecretSharedOT::recvRandomOTs(size_t numOTs, Socket &socket) {
    // Resize Vectors
    BitVector randomChoices(numOTs);
    vector<block> randomMsgs(numOTs);

    // Random OTs
    SilentOtExtReceiver receiver;
    receiver.mMultType = MultType::ExConv7x24;
    receiver.configure(numOTs, 2, mNumThreads);
    sync_wait(receiver.genSilentBaseOts(mPrng, socket, true));
    sync_wait(receiver.silentReceive(randomChoices, randomMsgs, mPrng, socket));

    // Check Timing Result
    if (mTimer != nullptr) {
        mTimer->setTimePoint("SecretSharedOT.receiverandomots.silentreceive");
    }

    mRandomChoices.append(randomChoices);
    mRandomMessages.insert(mRandomMessages.end(), randomMsgs.begin(),
                           randomMsgs.end());
}
}  // namespace fuzzypc::protocols
