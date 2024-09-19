/* 본 프로그램에 대한 저작권을 포함한 지적재산권은 삼성SDS(주)에 있으며,
삼성SDS(주)가 명시적으로 허용하지 않은 사용, 복사, 변경, 제3자에의 공개,
배포는 엄격히 금지되며, 삼성SDS(주)의 지적재산권 침해에 해당됩니다.
Copyright (c) 2023 Samsung SDS Co., Ltd. All Rights Reserved. Confidential.
All information including the intellectual and technical concepts contained
herein is, and remains the property of Samsung SDS Co. Ltd. Unauthorized use,
dissemination, or reproduction of this material is strictly forbidden unless
prior written permission is obtained from Samsung SDS Co. Ltd.
*/

#include <chrono>

#include "Common/Random.h"
#include "Common/Utils.h"
#include "FuzzyPcSender.h"

using namespace std;
using namespace coproto;
using namespace osuCrypto;
using namespace fuzzypc::common;
using namespace fuzzypc::protocols;

namespace fuzzypc {
void FuzzyPcSender::runCpsiAndPns(const vector<byte> &deduplicated,
                               const vector<uint64_t> &deduplicatedToRows,
                               BitVector &membershipShares,
                               vector<byte> &payloadShares, size_t featureIdx,
                               Socket &socket) {
    uint32_t numDeduplicated = deduplicated.size() / mHashByteSize;

    vector<byte> payloads(mPayloadByteSize * numDeduplicated);
    for (size_t i = 0; i < numDeduplicated; i++) {
        memcpy(payloads.data() + i * mPayloadByteSize,
               mRandomIds.data() + deduplicatedToRows[i] * mPayloadByteSize,
               mPayloadByteSize);
    }

    mStart = chrono::steady_clock::now();

    BitVector membershipSharesNotAligned;
    vector<byte> payloadSharedNotAligned;
    mCpsiSenderPtrs[featureIdx]->run(
        deduplicated, mHashByteSize, payloads, mPayloadByteSize,
        membershipSharesNotAligned, payloadSharedNotAligned, socket);

    mEnd = chrono::steady_clock::now();
    auto sec = chrono::duration_cast<chrono::milliseconds>(mEnd - mStart);
    mCpsiTime += sec.count();

    if (mTimer != nullptr) {
        mTimer->setTimePoint("fuzzypcsender.cpsi");
    }

    if (featureIdx == 0) {
        membershipShares = membershipSharesNotAligned;
        payloadShares = payloadSharedNotAligned;
        mGlobalIndexFixed = true;
    } else {
        uint32_t pnsInputByteSize = mPayloadByteSize + 1;
        vector<byte> pnsInputs(pnsInputByteSize * mNumBins);
        for (size_t i = 0; i < mNumBins; i++) {
            pnsInputs[i * pnsInputByteSize] =
                static_cast<byte>((uint8_t)membershipSharesNotAligned[i]);
            memcpy(pnsInputs.data() + i * pnsInputByteSize + 1,
                   payloadSharedNotAligned.data() + i * mPayloadByteSize,
                   mPayloadByteSize);
        }

        mStart = chrono::steady_clock::now();

        auto permutedShares =
            mPnsReceiverPtrs[featureIdx - 1]->run(pnsInputs, socket);
        mEnd = chrono::steady_clock::now();
        sec = chrono::duration_cast<chrono::milliseconds>(mEnd - mStart);
        mPnsTime += sec.count();

        membershipShares.resize(mNumBins);
        payloadShares.resize(mNumBins * mPayloadByteSize);
        for (size_t i = 0; i < mNumBins; i++) {
            membershipShares[i] =
                static_cast<uint8_t>(permutedShares[i * pnsInputByteSize]) & 1;
            memcpy(payloadShares.data() + i * mPayloadByteSize,
                   permutedShares.data() + i * pnsInputByteSize + 1,
                   mPayloadByteSize);
        }
    }

    if (mTimer != nullptr) {
        mTimer->setTimePoint("fuzzypcsender.pas");
    }
}

void FuzzyPcSender::run(const std::vector<std::vector<std::string>> &input,
                     oc::Socket &socket) {
    size_t numRows = input.size();
    size_t numFeatures = input[0].size();

    vector<BitVector> membershipShares(numFeatures);
    vector<vector<byte>> payloadShares(numFeatures);

    for (size_t i = 0; i < numFeatures; i++) {
        vector<string> column(numRows);
        for (size_t j = 0; j < numRows; j++) {
            column[j] = input[j][i];
        }
        vector<uint64_t> deduplicatedToRows;
        auto deduplicated =
            dedupAndHash(column, mHashByteSize, deduplicatedToRows);
        runCpsiAndPns(deduplicated, deduplicatedToRows, membershipShares[i],
                      payloadShares[i], i, socket);
    }

    array<vector<byte>, 2> secretSharedMessages;
    secretSharedMessages[0].resize(mNumBins * mPayloadByteSize);
    randomBytes(secretSharedMessages[0].data(), secretSharedMessages[0].size());
    secretSharedMessages[1] = payloadShares[0];

    mStart = chrono::steady_clock::now();
    for (size_t i = 0; i < numFeatures; i++) {
        auto result =
            mSecretSharedOtPtr->run(membershipShares[i], secretSharedMessages,
                                    mPayloadByteSize, socket);

        if (i < numFeatures - 1) {
            secretSharedMessages[0] = result;
            secretSharedMessages[1] = payloadShares[i + 1];
        } else {
            sync_wait(socket.send(move(result)));
        }
    }
    mEnd = chrono::steady_clock::now();
    auto sec = chrono::duration_cast<chrono::milliseconds>(mEnd - mStart);
    mAsTime += sec.count();

    if (mTimer != nullptr) {
        mTimer->setTimePoint("fuzzypcsender.evaluate");
    }
}

void FuzzyPcSender::run(const std::vector<std::vector<uint64_t>> &input,
                     oc::Socket &socket) {
    size_t numRows = input.size();
    size_t numFeatures = input[0].size();

    vector<BitVector> membershipShares(numFeatures);
    vector<vector<byte>> payloadShares(numFeatures);

    for (size_t i = 0; i < numFeatures; i++) {
        vector<uint64_t> column(numRows);
        for (size_t j = 0; j < numRows; j++) {
            column[j] = input[j][i];
        }
        vector<uint64_t> deduplicatedToRows;
        auto deduplicated =
            dedupAndHash(column, mHashByteSize, deduplicatedToRows);
        runCpsiAndPns(deduplicated, deduplicatedToRows, membershipShares[i],
                      payloadShares[i], i, socket);
    }

    array<vector<byte>, 2> secretSharedMessages;
    secretSharedMessages[0].resize(mNumBins * mPayloadByteSize);
    randomBytes(secretSharedMessages[0].data(), secretSharedMessages[0].size());
    secretSharedMessages[1] = payloadShares[0];

    mStart = chrono::steady_clock::now();
    for (size_t i = 0; i < numFeatures; i++) {
        auto result =
            mSecretSharedOtPtr->run(membershipShares[i], secretSharedMessages,
                                    mPayloadByteSize, socket);

        if (i < numFeatures - 1) {
            secretSharedMessages[0] = result;
            secretSharedMessages[1] = payloadShares[i + 1];
        } else {
            sync_wait(socket.send(move(result)));
        }
    }
    mEnd = chrono::steady_clock::now();
    auto sec = chrono::duration_cast<chrono::milliseconds>(mEnd - mStart);
    mAsTime += sec.count();

    if (mTimer != nullptr) {
        mTimer->setTimePoint("fuzzypcsender.evaluate");
    }
}

void FuzzyPcSender::preprocess(size_t maxNumItems, size_t numFeatures,
                            size_t payloadByteSize, oc::Socket &socket) {
    mStart = chrono::steady_clock::now();
    mCpsiSenderPtrs.resize(numFeatures);
    for (size_t i = 0; i < numFeatures; i++) {
        mCpsiSenderPtrs[i] = std::make_shared<CPSISender>(mNumThreads, nullptr);
        mCpsiSenderPtrs[i]->preprocess(socket);
    }
    mPnsReceiverPtrs.resize(numFeatures - 1);
    for (size_t i = 0; i < numFeatures - 1; i++) {
        size_t numPnsInputs = mCpsiSenderPtrs[i + 1]->getNumBins();
        if (i == 0) {
            mNumBins = numPnsInputs;
        } else {
            if (numPnsInputs != mNumBins) {
                throw invalid_argument(
                    "number of bins at each feature is different");
            }
        }
        mPnsReceiverPtrs[i] =
            std::make_shared<PnsReceiver>(mNumThreads, nullptr);
        mPnsReceiverPtrs[i]->configure(numPnsInputs, payloadByteSize + 1);
        mPnsReceiverPtrs[i]->sendRandomOTs(socket);
    }
    mSecretSharedOtPtr =
        std::make_shared<SecretSharedOT>(true, mNumThreads, nullptr);
    mSecretSharedOtPtr->preprocess(mNumBins * numFeatures, socket);

    mEnd = chrono::steady_clock::now();
    auto sec = chrono::duration_cast<chrono::milliseconds>(mEnd - mStart);
    mSetupTime += sec.count();

    if (mTimer != nullptr) {
        mTimer->setTimePoint("fuzzypcsender.preprocess");
    }
}
}  // namespace fuzzypc
