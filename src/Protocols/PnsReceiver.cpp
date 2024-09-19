/* 본 프로그램에 대한 저작권을 포함한 지적재산권은 삼성SDS(주)에 있으며,
삼성SDS(주)가 명시적으로 허용하지 않은 사용, 복사, 변경, 제3자에의 공개,
배포는 엄격히 금지되며, 삼성SDS(주)의 지적재산권 침해에 해당됩니다.
Copyright (c) 2023 Samsung SDS Co., Ltd. All Rights Reserved. Confidential.
All information including the intellectual and technical concepts contained
herein is, and remains the property of Samsung SDS Co. Ltd. Unauthorized use,
dissemination, or reproduction of this material is strictly forbidden unless
prior written permission is obtained from Samsung SDS Co. Ltd.
*/

#include "PnsReceiver.h"

#include <cryptoTools/Crypto/AES.h>
#include <libOTe/TwoChooseOne/Silent/SilentOtExtSender.h>

#include "Common/Random.h"

using namespace std;
using namespace oc;

namespace fuzzypc::protocols {
PnsReceiver::PnsReceiver(size_t numThreads, oc::Timer *timer)
    : mNumThreads(numThreads), mTimer(timer) {
    mPrng.SetSeed(toBlock(fuzzypc::common::random64()));
}

std::vector<std::byte> PnsReceiver::run(std::vector<std::byte> &input,
                                        oc::Socket &chl) {
    recvCorrection(chl);
    genRandomWires();
    sendCorrection(chl);
    sendMaskedInputs(input, chl);

    std::vector<std::byte> result;
    result.resize(mNetwork.numInputs_ * mInputByteSize);
    for (size_t i = 0; i < mNetwork.numInputs_; i++) {
        memcpy(result.data() + i * mInputByteSize, mOutputWires[i].data(),
               mInputByteSize);
    }

    return result;
}

void PnsReceiver::sendRandomOTs(oc::Socket &chl) {
    vector<array<block, 2>> randomTwoMsgs(mNumSwitches);
    mRandomTwoMessages.resize(mNumSwitches);

    SilentOtExtSender sender;
    sender.mMultType = MultType::ExConv7x24;
    sender.configure(mNumSwitches, 2, mNumThreads);

    if (mTimer != nullptr) {
        mTimer->setTimePoint("pasreceiver.gensilentbaseots.start");
    }

    sync_wait(sender.genSilentBaseOts(mPrng, chl, true));

    if (mTimer != nullptr) {
        mTimer->setTimePoint("pasreceiver.gensilentbaseots.end");
    }

    sync_wait(sender.silentSend(randomTwoMsgs, mPrng, chl));

    if (mTimer != nullptr) {
        mTimer->setTimePoint("pasreceiver.sendrandomots.silentsend");
    }

    AES aes(ZeroBlock);
    for (size_t i = 0; i < mNumSwitches; i++) {
        mRandomTwoMessages[i][0] = {randomTwoMsgs[i][0],
                                    aes.ecbEncBlock(randomTwoMsgs[i][0])};
        mRandomTwoMessages[i][1] = {randomTwoMsgs[i][1],
                                    aes.ecbEncBlock(randomTwoMsgs[i][1])};
    }

    if (mTimer != nullptr) {
        mTimer->setTimePoint("pasreceiver.sendrandomots.expand");
    }
}

void PnsReceiver::genRandomWires() {
    size_t numInputs = mNetwork.numInputs_;
    size_t numCols = mNetwork.numCols_;
    mInputWires.resize(numInputs);
    mOutputWires.resize(numInputs);
    mTwoMessages.resize(mNumSwitches);

    vector<block> previousWires(numInputs);
    for (size_t i = 0; i < numInputs; i++) {
        previousWires[i] = mPrng.get();
    }
    mInputWires = previousWires;

    size_t idx = 0;
    array<block, 4> wires;
    for (size_t i = 0; i < numCols; i++) {
        auto tmp = previousWires;
        for (size_t j = 0; j < numInputs / 2; j++) {
            auto s = mNetwork.switches_[i][j];
            if (s.isSwitched >= 0) {
                mTwoMessages[idx][0] = mRandomTwoMessages[idx][0];

                wires[0] = tmp[s.in0];
                wires[1] = tmp[s.in1];
                // wires[2] = mPrng.get();
                // wires[3] = mPrng.get();
                wires[2] = mTwoMessages[idx][0][0] ^ wires[0];
                wires[3] = mTwoMessages[idx][0][1] ^ wires[1];

                // Set output wires
                if (i == numCols - 1) {
                    mOutputWires[s.out0] = wires[2];
                    mOutputWires[s.out1] = wires[3];
                }
                if (numInputs % 2 == 1) {
                    if (s.out1 == numInputs - 1) {
                        mOutputWires[numInputs - 1] = wires[3];
                    }
                }

                /*
                Set messages for OT
                T0 = w0 ^ w2 || w1 ^ w3
                T1 = w1 ^ w2 || w0 ^ w3
                */
                // mTwoMessages[idx][0] = {wires[0] ^ wires[2], wires[1] ^
                // wires[3]};
                mTwoMessages[idx][1] = {wires[1] ^ wires[2],
                                        wires[0] ^ wires[3]};

                // Update previous wires
                previousWires[s.out0] = wires[2];
                previousWires[s.out1] = wires[3];
                idx++;
            }
        }
    }

    if (mTimer != nullptr) {
        mTimer->setTimePoint("pasreceiver.genrandomwires");
    }
}

void PnsReceiver::sendMaskedInputs(std::vector<std::byte> &input,
                                   oc::Socket &chl) {
    size_t numInputs = input.size() / mInputByteSize;
    if (numInputs != mNetwork.numInputs_) {
        throw invalid_argument("The size of input is wrong");
    }

    vector<byte> maskedInput = input;
    for (size_t i = 0; i < numInputs; i++) {
        for (size_t j = 0; j < mInputByteSize; j++) {
            maskedInput[i * mInputByteSize + j] ^=
                (byte)(*(mInputWires[i].data() + j));
        }
    }

    cp::sync_wait(chl.send(move(maskedInput)));

    if (mTimer != nullptr) {
        mTimer->setTimePoint("pasreceiver.sendmaskedinputs");
    }
}

void PnsReceiver::sendCorrection(oc::Socket &chl) {
    vector<byte> M1(2 * mNumSwitches * mInputByteSize);
    for (size_t i = 0; i < mNumSwitches; i++) {
        size_t idx = 2 * i * mInputByteSize;
        block m10 = mTwoMessages[i][1][0] ^ mRandomTwoMessages[i][1][0];
        block m11 = mTwoMessages[i][1][1] ^ mRandomTwoMessages[i][1][1];
        memcpy(M1.data() + idx, m10.data(), mInputByteSize);
        memcpy(M1.data() + idx + mInputByteSize, m11.data(), mInputByteSize);
    }
    cp::sync_wait(chl.send(move(M1)));
}

void PnsReceiver::recvCorrection(oc::Socket &chl) {
    BitVector correction(mNumSwitches);
    cp::sync_wait(chl.recv(correction));

    for (size_t i = 0; i < mNumSwitches; i++) {
        if (correction[i] == 1) {
            std::swap(mRandomTwoMessages[i][0], mRandomTwoMessages[i][1]);
        }
    }
}
}  // namespace fuzzypc::protocols