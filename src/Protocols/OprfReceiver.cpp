/* 본 프로그램에 대한 저작권을 포함한 지적재산권은 삼성SDS(주)에 있으며,
삼성SDS(주)가 명시적으로 허용하지 않은 사용, 복사, 변경, 제3자에의 공개,
배포는 엄격히 금지되며, 삼성SDS(주)의 지적재산권 침해에 해당됩니다.
Copyright (c) 2022 Samsung SDS Co., Ltd. All Rights Reserved. Confidential.
All information including the intellectual and technical concepts contained
herein is, and remains the property of Samsung SDS Co. Ltd. Unauthorized use,
dissemination, or reproduction of this material is strictly forbidden unless
prior written permission is obtained from Samsung SDS Co. Ltd.
*/

#include "OprfReceiver.h"

#include <coproto/coproto.h>
#include <libOTe/Vole/Silent/SilentVoleReceiver.h>

#include "Algorithms/Rr22Okvs/Rr22Okvs.h"
#include "Common/Random.h"
#include "Defines.h"

using namespace std;
using namespace coproto;
using namespace oc;
using namespace fuzzypc::algorithms;

namespace fuzzypc::protocols {
OprfReceiver::OprfReceiver(size_t numThreads, oc::Timer *timer)
    : mNumThreads(numThreads), mTimer(timer) {
    mPrng.SetSeed(oc::toBlock(fuzzypc::common::random64()));
}

void OprfReceiver::recvVectorOLE(size_t numInputs, Socket &chl) {
    // Construct OKVS without encoding to compute number of Vector OLEs
    Rr22Okvs okvs;
    block seed = mPrng.get();
    okvs.configure(numInputs, mGamma, 16, seed, mNumThreads);
    size_t numVoles = okvs.size();

    // Send the number of vector OLEs that we want to generate
    sync_wait(chl.send(std::move(numVoles)));

    // Resize
    mVoleA.resize(numVoles);
    mVoleC.resize(numVoles);

    // Run Vector OLE protocol
    SilentVoleReceiver<block, block, CoeffCtxGF128> receiver;
    receiver.mMultType = oc::MultType::ExConv7x24;
    receiver.configure(numVoles, oc::SilentBaseType::BaseExtend, 128);
    sync_wait(receiver.genSilentBaseOts(mPrng, chl));
    sync_wait(receiver.silentReceive(mVoleA, mVoleC, mPrng, chl));

    mGenerated = numVoles;

    if (mTimer != nullptr) {
        mTimer->setTimePoint("oprfreceiver.recvvectorole");
    }
}

vector<byte> OprfReceiver::recv(vector<byte> &inputs, u32 inputBitSize,
                                u32 outputBitSize, Socket &chl) {
    // Compute Parameters
    size_t inputByteSize = (inputBitSize + 7) / 8;
    size_t numInputs = inputs.size() / inputByteSize;

    // Encode inputs using OKVS algorithm
    vector<byte> encoded, zeroValues(numInputs * (COMP_SEC_PARAM / 8));
    oc::block seed = mPrng.get();
    {
        Rr22Okvs okvs;
        okvs.configure(numInputs, mGamma, COMP_SEC_PARAM / 8, seed,
                       mNumThreads);
        okvs.encode(inputs, zeroValues, inputByteSize, encoded);
    }
    size_t numVoles = encoded.size() / (COMP_SEC_PARAM / 8);

    if (numVoles > mGenerated) {
        throw invalid_argument("the number of generated voles is not enough");
    }

    if (mTimer != nullptr) {
        mTimer->setTimePoint("oprfreceiver.okvsencode");
    }

    // Send masked OKVS encoding
    vector<block> maskedOKVS(numVoles);
    for (size_t i = 0; i < numVoles; ++i) {
        block ithOKVSEncodingAsBlock;
        memcpy(&ithOKVSEncodingAsBlock, encoded.data() + sizeof(block) * i,
               sizeof(block));
        maskedOKVS[i] = mVoleA[i] ^ ithOKVSEncodingAsBlock;
    }

    sync_wait(chl.send(move(numVoles)));
    sync_wait(chl.send(move(numInputs)));
    sync_wait(chl.send(move(seed)));
    sync_wait(chl.send(move(maskedOKVS)));

    if (mTimer != nullptr) {
        mTimer->setTimePoint("oprfreceiver.maskingokvs");
    }

    // Run OKVS decode
    vector<byte> voleCAsByte(numVoles * sizeof(block));
    for (size_t i = 0; i < numVoles; i++) {
        memcpy(voleCAsByte.data() + i * sizeof(block), &mVoleC[i],
               sizeof(block));
    }
    vector<byte> decoded;
    {
        Rr22Okvs okvs;
        okvs.configure(numInputs, mGamma, COMP_SEC_PARAM / 8, seed,
                       mNumThreads);
        okvs.decode(voleCAsByte, inputs, inputByteSize, decoded);
    }

    if (mTimer != nullptr) {
        mTimer->setTimePoint("oprfreceiver.okvsdecode");
    }

    // Apply Random Oracle
    size_t outputByteSize = (outputBitSize + 7) / 8;
    vector<byte> result(numInputs * outputByteSize);
    RandomOracle randomOracle;
    for (size_t i = 0; i < numInputs; ++i) {
        randomOracle.Reset(outputByteSize);
        randomOracle.Update(
            reinterpret_cast<u8 *>(decoded.data()) + i * sizeof(block),
            sizeof(block));
        randomOracle.Final(reinterpret_cast<u8 *>(result.data()) +
                           i * outputByteSize);
    }

    if (mTimer != nullptr) {
        mTimer->setTimePoint("oprfreceiver.randomoracle");
    }

    return result;
}
}  // namespace fuzzypc::protocols