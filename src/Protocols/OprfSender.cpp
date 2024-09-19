/* 본 프로그램에 대한 저작권을 포함한 지적재산권은 삼성SDS(주)에 있으며,
삼성SDS(주)가 명시적으로 허용하지 않은 사용, 복사, 변경, 제3자에의 공개,
배포는 엄격히 금지되며, 삼성SDS(주)의 지적재산권 침해에 해당됩니다.
Copyright (c) 2023 Samsung SDS Co., Ltd. All Rights Reserved. Confidential.
All information including the intellectual and technical concepts contained
herein is, and remains the property of Samsung SDS Co. Ltd. Unauthorized use,
dissemination, or reproduction of this material is strictly forbidden unless
prior written permission is obtained from Samsung SDS Co. Ltd.
*/

#include "OprfSender.h"

#include <coproto/coproto.h>
#include <cryptoTools/Crypto/PRNG.h>
#include <cryptoTools/Crypto/RandomOracle.h>

#include "Algorithms/Rr22Okvs/Rr22Okvs.h"
#include "Common/Random.h"
#include "Defines.h"

using namespace std;
using namespace coproto;
using namespace oc;
using namespace fuzzypc::algorithms;

namespace fuzzypc::protocols {
OPRFSender::OPRFSender(size_t numThreads, Timer *timer)
    : mNumThreads(numThreads), mTimer(timer) {
    mPrng.SetSeed(toBlock(fuzzypc::common::random64()));
};

void OPRFSender::sendVectorOLE(Socket &chl) {
    // Receive number of Vector OLEs that we want to generate
    sync_wait(chl.recv(mGenerated));

    // Randomly pick delta and resize
    mDelta = mPrng.get();
    mVoleB.resize(mGenerated);

    // Run Vector OLE protocol
    SilentVoleSender<block, block, CoeffCtxGF128> sender;
    sender.mMultType = MultType::ExConv7x24;
    sender.configure(mGenerated, SilentBaseType::BaseExtend);
    sync_wait(sender.genSilentBaseOts(mPrng, chl, mDelta));
    sync_wait(sender.silentSend(mDelta, mVoleB, mPrng, chl));

    if (mTimer != nullptr) {
        mTimer->setTimePoint("oprfsender.sendvectorole");
    }
}

void OPRFSender::send(Socket &chl) {
    // Receive the number of voles in receiver's recv call
    size_t numVoles;
    array<size_t, 2> params;
    chl.recv(params);
    sync_wait(chl.recv(numVoles));
    sync_wait(chl.recv(mReceiverNumInputs));

    if (numVoles > mGenerated) {
        throw invalid_argument("the number of generated voles is not enough");
    }

    // Receive masked OKVS seed and encoding
    vector<block> maskedOKVS(numVoles);
    sync_wait(when_all_ready(chl.recv(mSeeds), chl.recv(maskedOKVS)));

    if (mTimer != nullptr) {
        mTimer->setTimePoint("oprfsender.waitforokvs");
    }

    // Compute OPRF key
    mKeys.resize(numVoles * sizeof(block));
    for (size_t i = 0; i < numVoles; i++) {
        block K = mVoleB[i] ^ mDelta.gf128Mul(maskedOKVS[i]);
        memcpy(mKeys.data() + i * sizeof(block), &K, sizeof(block));
    }

    if (mTimer != nullptr) {
        mTimer->setTimePoint("oprfsender.computekey");
    }
}

vector<byte> OPRFSender::encode(vector<byte> &inputs, u32 inputBitSize,
                                u32 outputBitSize) {
    size_t inputByteSize = (inputBitSize + 7) / 8;
    size_t numInputs = inputs.size() / inputByteSize;

    // Run OKVS decode
    vector<byte> decoded;
    Rr22Okvs okvs;
    okvs.configure(mReceiverNumInputs, mGamma, COMP_SEC_PARAM / 8, mSeeds,
                   mNumThreads);

    if (mKeys.size() != okvs.size() * sizeof(block)) {
        throw invalid_argument("the size of keys is wrong");
    }

    okvs.decode(mKeys, inputs, inputByteSize, decoded);

    if (mTimer != nullptr) {
        mTimer->setTimePoint("oprfsender.okvsdecode");
    }

    // Apply Random Oracle
    size_t outputByteSize = (outputBitSize + 7) / 8;
    vector<byte> output(numInputs * outputByteSize);
    RandomOracle randomOracle;
    for (size_t i = 0; i < numInputs; ++i) {
        randomOracle.Reset(outputByteSize);
        randomOracle.Update(
            reinterpret_cast<u8 *>(decoded.data()) + i * (COMP_SEC_PARAM / 8),
            (COMP_SEC_PARAM / 8));
        randomOracle.Final(reinterpret_cast<u8 *>(output.data()) +
                           i * outputByteSize);
    }

    if (mTimer != nullptr) {
        mTimer->setTimePoint("oprfsender.randomoracle");
    }

    return output;
}

vector<byte> OPRFSender::send(vector<byte> &inputs, u32 inputBitSize,
                              u32 outputBitSize, Socket &chl) {
    send(chl);
    return encode(inputs, inputBitSize, outputBitSize);
}
}  // namespace fuzzypc::protocols