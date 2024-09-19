/* 본 프로그램에 대한 저작권을 포함한 지적재산권은 삼성SDS(주)에 있으며,
삼성SDS(주)가 명시적으로 허용하지 않은 사용, 복사, 변경, 제3자에의 공개,
배포는 엄격히 금지되며, 삼성SDS(주)의 지적재산권 침해에 해당됩니다.
Copyright (c) 2023 Samsung SDS Co., Ltd. All Rights Reserved. Confidential.
All information including the intellectual and technical concepts contained
herein is, and remains the property of Samsung SDS Co. Ltd. Unauthorized use,
dissemination, or reproduction of this material is strictly forbidden unless
prior written permission is obtained from Samsung SDS Co. Ltd.
*/

#include "PnsSender.h"

#include <cryptoTools/Crypto/AES.h>
#include <libOTe/TwoChooseOne/Silent/SilentOtExtReceiver.h>

#include "Common/Random.h"

using namespace std;
using namespace oc;

namespace fuzzypc::protocols {
PnsSender::PnsSender(size_t numThreads, Timer *timer)
    : mNumThreads(numThreads), mTimer(timer) {
    mPrng.SetSeed(toBlock(fuzzypc::common::random64()));
}

std::vector<std::byte> PnsSender::run(IntegerPermutation &perm,
                                      oc::Socket &chl) {
    mNetwork.routing(perm);

    sendCorrection(chl);
    recvCorrection(chl);

    std::vector<std::byte> masked;
    masked.resize(mNetwork.numInputs_ * mInputByteSize);
    cp::sync_wait(chl.recv(masked));

    std::vector<std::byte> result;
    evaluateBytes(masked, result);

    return result;
}

void PnsSender::recvRandomOTs(Socket &chl) {
    vector<block> randomMsgs(mNumSwitches);
    mRandomChoices.resize(mNumSwitches);
    mRandomMessages.resize(mNumSwitches);

    SilentOtExtReceiver receiver;
    receiver.mMultType = MultType::ExConv7x24;
    receiver.configure(mNumSwitches, 2, mNumThreads);
    sync_wait(receiver.genSilentBaseOts(mPrng, chl, true));
    sync_wait(receiver.silentReceive(mRandomChoices, randomMsgs, mPrng, chl));

    if (mTimer != nullptr) {
        mTimer->setTimePoint("passender.receiverandomots.silentreceive");
    }

    AES aes(ZeroBlock);
    for (size_t i = 0; i < mNumSwitches; i++) {
        mRandomMessages[i] = {randomMsgs[i], aes.ecbEncBlock(randomMsgs[i])};
    }

    if (mTimer != nullptr) {
        mTimer->setTimePoint("passender.receiverandomots.expand");
    }
}

void PnsSender::sendCorrection(Socket &chl) {
    size_t numInputs = mNetwork.numInputs_;
    size_t numCols = mNetwork.numCols_;

    BitVector correction(mNumSwitches);
    size_t idx = 0;
    for (size_t i = 0; i < numCols; i++) {
        for (size_t j = 0; j < numInputs / 2; j++) {
            auto s = mNetwork.switches_[i][j];
            if (s.isSwitched >= 0) {
                correction[idx] = s.isSwitched;
                idx++;
            }
        }
    }
    correction ^= mRandomChoices;
    cp::sync_wait(chl.send(move(correction)));
}

void PnsSender::recvCorrection(Socket &chl) {
    size_t numInputs = mNetwork.numInputs_;
    size_t numCols = mNetwork.numCols_;

    vector<byte> M1(2 * mNumSwitches * mInputByteSize);
    cp::sync_wait(chl.recv(M1));

    size_t idx = 0;
    mMessages.resize(mNumSwitches, {ZeroBlock, ZeroBlock});
    for (size_t i = 0; i < numCols; i++) {
        for (size_t j = 0; j < numInputs / 2; j++) {
            auto s = mNetwork.switches_[i][j];
            if (s.isSwitched >= 0) {
                if (s.isSwitched == 1) {
                    std::memcpy(mMessages[idx][0].data(),
                                M1.data() + idx * 2 * mInputByteSize,
                                mInputByteSize);
                    std::memcpy(
                        mMessages[idx][1].data(),
                        M1.data() + idx * 2 * mInputByteSize + mInputByteSize,
                        mInputByteSize);
                }
                mMessages[idx][0] ^= mRandomMessages[idx][0];
                mMessages[idx][1] ^= mRandomMessages[idx][1];
                idx++;
            }
        }
    }
}

void PnsSender::evaluateBlock(vector<block> &input, vector<block> &output) {
    size_t numInputs = mNetwork.numInputs_;
    size_t numCols = mNetwork.numCols_;

    size_t idx = 0;
    output = input;
    for (size_t i = 0; i < numCols; i++) {
        auto tmp = output;
        for (size_t j = 0; j < numInputs / 2; j++) {
            auto s = mNetwork.switches_[i][j];
            if (s.isSwitched == 1) {
                output[s.out0] = tmp[s.in1] ^ mMessages[idx][0];
                output[s.out1] = tmp[s.in0] ^ mMessages[idx][1];
                idx++;
            } else if (s.isSwitched == 0) {
                output[s.out0] = tmp[s.in0] ^ mMessages[idx][0];
                output[s.out1] = tmp[s.in1] ^ mMessages[idx][1];
                idx++;
            }
        }
    }
}

void PnsSender::evaluateBytes(vector<byte> &input, vector<byte> &output) {
    vector<block> inputAsBlock(mNetwork.numInputs_, ZeroBlock), outputAsBlock;
    for (size_t i = 0; i < mNetwork.numInputs_; i++) {
        memcpy(inputAsBlock[i].data(), input.data() + i * mInputByteSize,
               mInputByteSize);
    }
    evaluateBlock(inputAsBlock, outputAsBlock);
    output.resize(mNetwork.numInputs_ * mInputByteSize);
    for (size_t i = 0; i < mNetwork.numInputs_; i++) {
        memcpy(output.data() + i * mInputByteSize, outputAsBlock[i].data(),
               mInputByteSize);
    }

    if (mTimer != nullptr) {
        mTimer->setTimePoint("passender.evaluate");
    }
}
}  // namespace fuzzypc::protocols