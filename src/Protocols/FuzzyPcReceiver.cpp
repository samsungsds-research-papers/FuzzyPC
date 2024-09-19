/* 본 프로그램에 대한 저작권을 포함한 지적재산권은 삼성SDS(주)에 있으며,
삼성SDS(주)가 명시적으로 허용하지 않은 사용, 복사, 변경, 제3자에의 공개,
배포는 엄격히 금지되며, 삼성SDS(주)의 지적재산권 침해에 해당됩니다.
Copyright (c) 2023 Samsung SDS Co., Ltd. All Rights Reserved. Confidential.
All information including the intellectual and technical concepts contained
herein is, and remains the property of Samsung SDS Co. Ltd. Unauthorized use,
dissemination, or reproduction of this material is strictly forbidden unless
prior written permission is obtained from Samsung SDS Co. Ltd.
*/

#include "FuzzyPcReceiver.h"

#include "Common/Random.h"
#include "Common/Utils.h"

#include <xxh3.h>

using namespace std;
using namespace coproto;
using namespace osuCrypto;
using namespace fuzzypc::common;
using namespace fuzzypc::protocols;
using namespace fuzzypc::structures;

namespace fuzzypc {
void FuzzyPcReceiver::runCpsiAndPns(const vector<byte> &deduplicated,
                                 const vector<uint64_t> &deduplicatedToRows,
                                 BitVector &membershipShares,
                                 vector<byte> &payloadShares, size_t featureIdx,
                                 Socket &socket) {
    uint32_t numDeduplicated = deduplicated.size() / mHashByteSize;

    BitVector membershipSharesNoAlign;
    vector<byte> payloadSharesNoAlign;
    vector<size_t> deduplicatedToTable;
    mCpsiReceiverPtrs[featureIdx]->run(
        deduplicated, mHashByteSize, mPayloadByteSize, membershipSharesNoAlign,
        payloadSharesNoAlign, deduplicatedToTable, socket);

    if (mTimer != nullptr) {
        mTimer->setTimePoint("fuzzypcreceiver.cpsi");
    }

    if (mGlobalIndex.size() == 0) {
        vector<bool> binChecker(mNumBins, false);
        vector<bool> recordChecker(mNumRows, false);
        mGlobalIndex.resize(mNumRows);
        size_t binIndex, rowIndex;
        for (size_t i = 0; i < numDeduplicated; i++) {
            binIndex = deduplicatedToTable[i];
            rowIndex = deduplicatedToRows[i];
            mGlobalIndex[rowIndex] = binIndex;
            binChecker[binIndex] = true;
            recordChecker[rowIndex] = true;
        }

        vector<size_t> blackIndices;
        for (size_t i = 0; i < mNumBins; i++) {
            if (!binChecker[i]) {
                blackIndices.push_back(i);
            }
        }
        random_shuffle(blackIndices.begin(), blackIndices.end());

        for (size_t i = 0; i < mNumRows; i++) {
            if (!recordChecker[i]) {
                mGlobalIndex[i] = blackIndices.back();
                blackIndices.pop_back();
            }
        }
        membershipShares = membershipSharesNoAlign;
        payloadShares = payloadSharesNoAlign;
    } else {
        vector<bool> rangeChecker(mNumBins);
        vector<bool> domainChecker(mNumBins);
        IntegerPermutation permutationInv(0, mNumBins - 1);
        uint64_t binIndex, rowIndex;
        for (size_t i = 0; i < numDeduplicated; i++) {
            binIndex = deduplicatedToTable[i];
            rowIndex = deduplicatedToRows[i];
            permutationInv.set(mGlobalIndex[rowIndex], binIndex);
            rangeChecker[binIndex] = true;
            domainChecker[mGlobalIndex[rowIndex]] = true;
        }

        vector<uint64_t> blackIndices;
        for (size_t i = 0; i < mNumBins; i++) {
            if (!rangeChecker[i]) {
                blackIndices.push_back(i);
            }
        }
        random_shuffle(blackIndices.begin(), blackIndices.end());

        for (size_t i = 0; i < mNumBins; i++) {
            if (!domainChecker[i]) {
                permutationInv.set(i, blackIndices.back());
                blackIndices.pop_back();
            }
        }

        uint32_t pnsByteSize = mPayloadByteSize + 1;
        vector<byte> sharesInByte(pnsByteSize * mNumBins);
        for (size_t i = 0; i < mNumBins; i++) {
            sharesInByte[i * pnsByteSize] =
                static_cast<byte>((uint8_t)membershipSharesNoAlign[i]);
            memcpy(sharesInByte.data() + i * pnsByteSize + 1,
                   payloadSharesNoAlign.data() + i * mPayloadByteSize,
                   mPayloadByteSize);
        }

        auto permutation = permutationInv.inverse();
        auto permutedShares =
            mPnsSenderPtrs[featureIdx - 1]->run(permutation, socket);

        for (size_t i = 0; i < mNumBins; i++) {
            for (size_t k = 0; k < pnsByteSize; k++) {
                permutedShares[i * pnsByteSize + k] ^=
                    sharesInByte[permutationInv.get(i) * pnsByteSize + k];
            }
        }

        membershipShares.resize(mNumBins);
        payloadShares.resize(mNumBins * mPayloadByteSize);
        for (size_t i = 0; i < mNumBins; i++) {
            membershipShares[i] =
                static_cast<uint8_t>(permutedShares[i * pnsByteSize]) & 1;
            memcpy(payloadShares.data() + i * mPayloadByteSize,
                   permutedShares.data() + i * pnsByteSize + 1,
                   mPayloadByteSize);
        }
    }

    if (mTimer != nullptr) {
        mTimer->setTimePoint("fuzzypcreceiver.pns");
    }
}

vector<byte> FuzzyPcReceiver::run(const vector<vector<string>> &input,
                               oc::Socket &socket) {
    size_t numRows = input.size();
    size_t numFeatures = input[0].size();

    vector<BitVector> membershipShares(numFeatures);
    vector<vector<byte>> payloadSharesInByte(numFeatures);

    for (size_t i = 0; i < numFeatures; i++) {
        vector<byte> deduplicated;
        vector<uint64_t> deduplicatedToRows;
        vector<string> column(numRows);
        for (size_t j = 0; j < numRows; j++) {
            column[j] = input[j][i];
        }
        deduplicated = dedupAndHash(column, mHashByteSize, deduplicatedToRows);
        runCpsiAndPns(deduplicated, deduplicatedToRows, membershipShares[i],
                      payloadSharesInByte[i], i, socket);
    }

    array<vector<byte>, 2> secretSharedMessages;
    secretSharedMessages[0].resize(mNumBins * mPayloadByteSize);
    randomBytes(secretSharedMessages[0].data(), secretSharedMessages[0].size());
    secretSharedMessages[1] = payloadSharesInByte[0];

    vector<byte> result;
    for (size_t i = 0; i < numFeatures; i++) {
        auto secretSharedOTResult =
            mSecretSharedOtPtr->run(membershipShares[i], secretSharedMessages,
                                 mPayloadByteSize, socket);
        if (i < numFeatures - 1) {
            secretSharedMessages[0] = secretSharedOTResult;
            secretSharedMessages[1] = payloadSharesInByte[i + 1];
        } else {  // the last call
            vector<byte> theirResultInByte(mNumBins * mPayloadByteSize);
            sync_wait(socket.recv(theirResultInByte));
            result.resize(mNumBins * mPayloadByteSize);
            for (size_t k = 0; k < mNumBins * mPayloadByteSize; k++) {
                result[k] = secretSharedOTResult[k] ^ theirResultInByte[k];
            }
        }
    }

    if (mTimer != nullptr) {
        mTimer->setTimePoint("fuzzypcreceiver.evaluate");
    }

    vector<byte> payloadsInByte(mNumRows * mPayloadByteSize);
    for (size_t i = 0; i < mNumRows; i++) {
        memcpy(payloadsInByte.data() + i * mPayloadByteSize,
               result.data() + mGlobalIndex[i] * mPayloadByteSize,
               mPayloadByteSize);
    }

    return payloadsInByte;
}

vector<byte> FuzzyPcReceiver::run(const vector<vector<uint64_t>> &input,
                               oc::Socket &socket) {
    size_t numRows = input.size();
    size_t numFeatures = input[0].size();

    vector<BitVector> membershipShares(numFeatures);
    vector<vector<byte>> payloadSharesInByte(numFeatures);

    for (size_t i = 0; i < numFeatures; i++) {
        vector<byte> deduplicated;
        vector<uint64_t> deduplicatedToRows;
        vector<uint64_t> column(numRows);
        for (size_t j = 0; j < numRows; j++) {
            column[j] = input[j][i];
        }
        deduplicated = dedupAndHash(column, mHashByteSize, deduplicatedToRows);
        runCpsiAndPns(deduplicated, deduplicatedToRows, membershipShares[i],
                      payloadSharesInByte[i], i, socket);
    }

    array<vector<byte>, 2> secretSharedMessages;
    secretSharedMessages[0].resize(mNumBins * mPayloadByteSize);
    randomBytes(secretSharedMessages[0].data(), secretSharedMessages[0].size());
    secretSharedMessages[1] = payloadSharesInByte[0];

    vector<byte> result;
    for (size_t i = 0; i < numFeatures; i++) {
        auto secretSharedOTResult =
            mSecretSharedOtPtr->run(membershipShares[i], secretSharedMessages,
                                 mPayloadByteSize, socket);
        if (i < numFeatures - 1) {
            secretSharedMessages[0] = secretSharedOTResult;
            secretSharedMessages[1] = payloadSharesInByte[i + 1];
        } else {  // the last call
            vector<byte> theirResultInByte(mNumBins * mPayloadByteSize);
            sync_wait(socket.recv(theirResultInByte));
            result.resize(mNumBins * mPayloadByteSize);
            for (size_t k = 0; k < mNumBins * mPayloadByteSize; k++) {
                result[k] = secretSharedOTResult[k] ^ theirResultInByte[k];
            }
        }
    }

    if (mTimer != nullptr) {
        mTimer->setTimePoint("fuzzypcreceiver.evaluate");
    }

    vector<byte> payloadsInByte(mNumRows * mPayloadByteSize);
    for (size_t i = 0; i < mNumRows; i++) {
        memcpy(payloadsInByte.data() + i * mPayloadByteSize,
               result.data() + mGlobalIndex[i] * mPayloadByteSize,
               mPayloadByteSize);
    }

    return payloadsInByte;
}

void FuzzyPcReceiver::preprocess(size_t maxNumItems, size_t numFeatures,
                              size_t idByteSize, oc::Socket &chl) {
    mCpsiReceiverPtrs.resize(numFeatures);
    for (size_t i = 0; i < numFeatures; i++) {
        mCpsiReceiverPtrs[i] = make_shared<CpsiReceiver>(mNumThreads);
        mCpsiReceiverPtrs[i]->preprocess(maxNumItems, chl);
    }
    mPnsSenderPtrs.resize(numFeatures - 1);
    for (size_t i = 0; i < numFeatures - 1; i++) {
        size_t numPnsInputs = mCpsiReceiverPtrs[i + 1]->getNumBins();
        if (i == 0) {
            mNumBins = numPnsInputs;
        } else {
            if (numPnsInputs != mNumBins) {
                throw invalid_argument(
                    "number of bins at each feature is different");
            }
        }
        mPnsSenderPtrs[i] = make_shared<PnsSender>(mNumThreads, nullptr);
        mPnsSenderPtrs[i]->configure(numPnsInputs, idByteSize + 1);
        mPnsSenderPtrs[i]->recvRandomOTs(chl);
    }
    mSecretSharedOtPtr = make_shared<SecretSharedOT>(false, mNumThreads, nullptr);
    mSecretSharedOtPtr->preprocess(mNumBins * numFeatures, chl);

    if (mTimer != nullptr) {
        mTimer->setTimePoint("pprlreceiver.preprocess");
    }
}

}  // namespace fuzzypc