/* 본 프로그램에 대한 저작권을 포함한 지적재산권은 삼성SDS(주)에 있으며,
삼성SDS(주)가 명시적으로 허용하지 않은 사용, 복사, 변경, 제3자에의 공개,
배포는 엄격히 금지되며, 삼성SDS(주)의 지적재산권 침해에 해당됩니다.
Copyright (c) 2023 Samsung SDS Co., Ltd. All Rights Reserved. Confidential.
All information including the intellectual and technical concepts contained
herein is, and remains the property of Samsung SDS Co. Ltd. Unauthorized use,
dissemination, or reproduction of this material is strictly forbidden unless
prior written permission is obtained from Samsung SDS Co. Ltd.
*/

#include "EqualityCheck.h"
#include "Common/Random.h"

using namespace std;
using namespace coproto;
using namespace oc;

namespace fuzzypc::protocols {
EqualityCheck::EqualityCheck(bool isSender, size_t numThreads, oc::Timer *timer)
    : mIsSender(isSender), mNumThreads(numThreads), mTimer(timer) {
    mSilentOtExtSenderPtr = make_shared<SilentOtExtSender>();
    mSilentOtExtReceiverPtr = make_shared<SilentOtExtReceiver>();
    mPrng.SetSeed(oc::toBlock(fuzzypc::common::random64()));
}

BitVector EqualityCheck::run(std::vector<std::byte> &inputs,
                             size_t inputBitSize, oc::Socket &chl) {
    // Compute Parameters
    size_t inputByteSize = (inputBitSize + 7) / 8;
    size_t numInputs = inputs.size() / inputByteSize;
    size_t numOTs = 2 * (inputBitSize - 1) * numInputs;

    // Call Random OT if needed
    if (mRemainRandomOt < numOTs) {
        if (mIsSender) {
            sendRandomOTs(numOTs - mRemainRandomOt, chl);
        } else {
            recvRandomOTs(numOTs - mRemainRandomOt, chl);
        }
    }

    // Check Timing Result
    if (mTimer != nullptr) {
        mTimer->setTimePoint("equalitycheck.randomot");
    }

    // Decompose Inputs
    vector<BitVector> acc(inputBitSize);
    for (size_t i = 0; i < inputBitSize; i++) {
        acc[i].resize(numInputs);
    }
    for (size_t i = 0; i < numInputs; i++) {
        BitVector decompose(
            reinterpret_cast<u8 *>(inputs.data()) + i * inputByteSize,
            8 * inputByteSize);
        if (mIsSender) {  // flip if sender
            decompose = ~decompose;
        }
        for (size_t j = 0; j < inputBitSize; j++) {
            acc[j][i] = decompose[j];
        }
    }

    // Check Timing Result
    if (mTimer != nullptr) {
        mTimer->setTimePoint("equalitycheck.inputdecompose");
    }

    // Compute Equality Check Circuit using ANDs
    while (acc.size() > 1) {
        BitVector left, right;
        for (size_t i = 0; i < acc.size() / 2; i++) {
            left.append(acc[2 * i]);
            right.append(acc[2 * i + 1]);
        }
        BitVector rightAndLeft, leftAndRight, result;
        if (mIsSender) {
            computeAND(right, rightAndLeft, chl);
            computeAND(left, leftAndRight, chl);
        } else {
            computeAND(left, leftAndRight, chl);
            computeAND(right, rightAndLeft, chl);
        }
        result = (left & right) ^ rightAndLeft ^ leftAndRight;

        // Copy Result to acc
        for (size_t i = 0; i < acc.size() / 2; i++) {
            acc[i].copy(result, i * numInputs, numInputs);
        }

        // Resize acc
        if (acc.size() % 2 != 0) {
            acc[acc.size() / 2] = acc[acc.size() - 1];
            acc.resize((acc.size() / 2) + 1);
        } else {
            acc.resize((acc.size() / 2));
        }
    }

    // Check Timing Result
    if (mTimer != nullptr) {
        mTimer->setTimePoint("equalitycheck.evaluate");
    }

    return acc[0];
}

void EqualityCheck::computeAND(const oc::BitVector &input,
                               oc::BitVector &output, oc::Socket &chl) {
    BitVector correction, r0, r1, r2;

    if (mIsSender) {
        // Receive Correction
        correction.resize(input.size());
        sync_wait(chl.recv(correction));

        // Get Two Random Messages
        r0.append(mRandomTwoMessages[0].data(), input.size(), mRandomOtIdx);
        r1.append(mRandomTwoMessages[1].data(), input.size(), mRandomOtIdx);

        // Compute and Send Correction for Correlated OTs
        r2 = (r0 ^ r1) & correction;
        r0 = r2 ^ r0;
        r1 = r2 ^ r1;
        r2 = input ^ r0 ^ r1;
        sync_wait(chl.send(move(r2)));

        // Update Parameters
        output = r0;
    } else {
        // Compute and Send Correction for Input
        correction.append(mRandomChoices.data(), input.size(), mRandomOtIdx);
        r0.append(mRandomMessages.data(), input.size(), mRandomOtIdx);
        correction ^= input;
        sync_wait(chl.send(move(correction)));

        // Receive Correction for Correlated OTs
        correction.resize(input.size());
        sync_wait(chl.recv(correction));

        // Update Parameters
        output = r0 ^ (input & correction);
    }

    mRandomOtIdx += input.size();
    mRemainRandomOt -= -input.size();
}

void EqualityCheck::sendRandomOTs(size_t numOTs, oc::Socket &chl) {
    // Resize Vectors
    vector<array<block, 2>> randomMsgsAsBlock(numOTs);
    array<BitVector, 2> tmp;
    tmp[0].resize(numOTs);
    tmp[1].resize(numOTs);

    // Random OTs
    mSilentOtExtSenderPtr->mMultType = MultType::ExConv7x24;
    mSilentOtExtSenderPtr->configure(numOTs, 2, mNumThreads);
    sync_wait(mSilentOtExtSenderPtr->genSilentBaseOts(mPrng, chl, true));
    sync_wait(mSilentOtExtSenderPtr->silentSend(randomMsgsAsBlock, mPrng, chl));

    // Check Timing Result
    if (mTimer != nullptr) {
        mTimer->setTimePoint("equalitycheck.sendrandomots.silentsend");
    }

    // Convert from block to BitVector
    for (size_t i = 0; i < numOTs; i++) {
        tmp[0][i] = randomMsgsAsBlock[i][0].mData[0] & 1;
        tmp[1][i] = randomMsgsAsBlock[i][1].mData[0] & 1;
    }
    mRandomTwoMessages[0].append(tmp[0]);
    mRandomTwoMessages[1].append(tmp[1]);
    mRemainRandomOt += numOTs;
}

void EqualityCheck::recvRandomOTs(size_t numOTs, Socket &chl) {
    // Resize Vectors
    BitVector randomChoices(numOTs), tmp(numOTs);
    vector<block> randomMsgsAsBlock(numOTs);

    // Random OTs
    mSilentOtExtReceiverPtr->mMultType = MultType::ExConv7x24;
    mSilentOtExtReceiverPtr->configure(numOTs, 2, mNumThreads);

    if (mTimer != nullptr) {
        mTimer->setTimePoint("equalitycheck.gensilentbaseots.start");
    }

    sync_wait(mSilentOtExtReceiverPtr->genSilentBaseOts(mPrng, chl, true));

    if (mTimer != nullptr) {
        mTimer->setTimePoint("equalitycheck.gensilentbaseots.end");
    }

    sync_wait(mSilentOtExtReceiverPtr->silentReceive(randomChoices, randomMsgsAsBlock,
                                            mPrng, chl));

    // Check Timing Result
    if (mTimer != nullptr) {
        mTimer->setTimePoint("equalitycheck.receiverandomots.silentreceive");
    }

    // Convert from block to BitVector
    for (size_t i = 0; i < numOTs; i++) {
        tmp[i] = randomMsgsAsBlock[i].mData[0] & 1;
    }
    mRandomMessages.append(tmp);
    mRandomChoices.append(randomChoices);
    mRemainRandomOt += numOTs;
}
}  // namespace fuzzypc::protocols