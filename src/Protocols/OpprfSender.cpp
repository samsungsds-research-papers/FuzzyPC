/* 본 프로그램에 대한 저작권을 포함한 지적재산권은 삼성SDS(주)에 있으며,
삼성SDS(주)가 명시적으로 허용하지 않은 사용, 복사, 변경, 제3자에의 공개,
배포는 엄격히 금지되며, 삼성SDS(주)의 지적재산권 침해에 해당됩니다.
Copyright (c) 2023 Samsung SDS Co., Ltd. All Rights Reserved. Confidential.
All information including the intellectual and technical concepts contained
herein is, and remains the property of Samsung SDS Co. Ltd. Unauthorized use,
dissemination, or reproduction of this material is strictly forbidden unless
prior written permission is obtained from Samsung SDS Co. Ltd.
*/

#include "OpprfSender.h"

#include "Algorithms/Rr22Okvs/Rr22Okvs.h"
#include "Common/Random.h"
#include "OprfSender.h"

using namespace std;
using namespace osuCrypto;
using namespace coproto;
using namespace fuzzypc::algorithms;

namespace fuzzypc::protocols {
OPPRFSender::OPPRFSender(size_t numThreads, Timer *timer)
    : mNumThreads(numThreads), mTimer(timer) {
    mOprfSenderPtr = make_shared<OPRFSender>(numThreads, timer);
    mPrng.SetSeed(toBlock(fuzzypc::common::random64()));
}

void OPPRFSender::sendVectorOLE(Socket &chl) {
    mOprfSenderPtr->sendVectorOLE(chl);
}

void OPPRFSender::send(vector<byte> &inputs, vector<byte> &values,
                       size_t inputByteSize, size_t valueByteSize,
                       Socket &chl) {
    // Compute parameters
    size_t inputBitSize = 8 * inputByteSize;
    size_t valueBitSize = 8 * valueByteSize;
    size_t numInputs = inputs.size() / inputByteSize;
    size_t numValues = values.size() / valueByteSize;
    if (numInputs != numValues) {
        throw invalid_argument("numInputs and numValues are different");
    }

    mKeys = mOprfSenderPtr->send(inputs, inputBitSize, valueBitSize, chl);
    if (mKeys.size() != values.size()) {
        throw invalid_argument("mKeys.size() and values.size() are different");
    }

    vector<byte> maskedValues(values);
    for (size_t i = 0; i < maskedValues.size(); i++) {
        maskedValues[i] ^= mKeys[i];
    }

    Rr22Okvs okvs;
    mSeed = mPrng.get();
    okvs.configure(numInputs, mGamma, valueByteSize, mSeed, mNumThreads);
    okvs.encode(inputs, maskedValues, inputByteSize, mMaskedOkvs);
    sync_wait(when_all_ready(chl.send(move(mSeed)), chl.send(move(mMaskedOkvs)),
                             chl.send(move(numInputs))));
}
}  // namespace fuzzypc::protocols