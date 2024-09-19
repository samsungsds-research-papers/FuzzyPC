/* 본 프로그램에 대한 저작권을 포함한 지적재산권은 삼성SDS(주)에 있으며,
삼성SDS(주)가 명시적으로 허용하지 않은 사용, 복사, 변경, 제3자에의 공개,
배포는 엄격히 금지되며, 삼성SDS(주)의 지적재산권 침해에 해당됩니다.
Copyright (c) 2023 Samsung SDS Co., Ltd. All Rights Reserved. Confidential.
All information including the intellectual and technical concepts contained
herein is, and remains the property of Samsung SDS Co. Ltd. Unauthorized use,
dissemination, or reproduction of this material is strictly forbidden unless
prior written permission is obtained from Samsung SDS Co. Ltd.
*/

#include "OpprfReceiver.h"

#include "Algorithms/Rr22Okvs/Rr22Okvs.h"
#include "Common/Random.h"
#include "OprfReceiver.h"

using namespace std;
using namespace coproto;
using namespace osuCrypto;
using namespace fuzzypc::algorithms;

namespace fuzzypc::protocols {
OPPRFReceiver::OPPRFReceiver(size_t numThreads, Timer *timer)
    : mNumThreads(numThreads), mTimer(timer) {
    mOprfReceiverPtr = make_shared<OprfReceiver>(numThreads, timer);
    mPrng.SetSeed(toBlock(fuzzypc::common::random64()));
}

void OPPRFReceiver::recvVectorOLE(size_t numInputs, Socket &chl) {
    mOprfReceiverPtr->recvVectorOLE(numInputs, chl);
}

vector<byte> OPPRFReceiver::recv(vector<byte> &inputs, size_t inputByteSize,
                                 size_t outputByteSize, Socket &chl) {
    // Compute parameters
    size_t inputBitSize = 8 * inputByteSize;
    size_t outputBitSize = 8 * outputByteSize;

    // OprfReceiver receiver(mNumThreads, mTimer);
    auto result1 =
        mOprfReceiverPtr->recv(inputs, inputBitSize, outputBitSize, chl);

    size_t senderNumInputs;
    vector<byte> maskedOKVS;
    sync_wait(when_all_ready(chl.recv(mSeed), chl.recvResize(maskedOKVS),
                             chl.recv(senderNumInputs)));

    if (mTimer != nullptr) {
        mTimer->setTimePoint("opprfreceiver.recvokvs");
    }

    vector<byte> result2;
    Rr22Okvs okvs;
    okvs.configure(senderNumInputs, mGamma, outputByteSize, mSeed, mNumThreads);
    okvs.decode(maskedOKVS, inputs, inputByteSize, result2);
    if (result1.size() != result2.size()) {
        throw invalid_argument(
            "result1.size() and result2.size() are different at " +
            string(__FILE__));
    }

    for (size_t i = 0; i < result1.size(); i++) {
        result1[i] ^= result2[i];
    }

    if (mTimer != nullptr) {
        mTimer->setTimePoint("opprfreceiver.okvsdecode");
    }

    return result1;
}
}  // namespace fuzzypc::protocols