/* 본 프로그램에 대한 저작권을 포함한 지적재산권은 삼성SDS(주)에 있으며,
삼성SDS(주)가 명시적으로 허용하지 않은 사용, 복사, 변경, 제3자에의 공개,
배포는 엄격히 금지되며, 삼성SDS(주)의 지적재산권 침해에 해당됩니다.
Copyright (c) 2023 Samsung SDS Co., Ltd. All Rights Reserved. Confidential.
All information including the intellectual and technical concepts contained
herein is, and remains the property of Samsung SDS Co. Ltd. Unauthorized use,
dissemination, or reproduction of this material is strictly forbidden unless
prior written permission is obtained from Samsung SDS Co. Ltd.
*/

#include "CpsiSender.h"

#include <kuku/kuku.h>
#include <libOTe/Vole/Silent/SilentVoleSender.h>

#include "Common/Random.h"

using namespace std;
using namespace kuku;
using namespace coproto;
using namespace osuCrypto;
using namespace fuzzypc::common;

namespace fuzzypc::protocols {
void CPSISender::run(const vector<byte> &items, const uint32_t itemByteSize,
                     const vector<byte> &payloads,
                     const uint32_t payloadByteSize,
                     BitVector &membershipShares, vector<byte> &payloadShares,
                     Socket &chl) {
    if (items.size() % itemByteSize != 0) {
        throw invalid_argument("size of items is not divided by itemByteSize");
    }
    size_t numItems = items.size() / itemByteSize;

    // Convert items to Kuku type
    vector<item_type> itemsAsKukuType(numItems);
    for (size_t i = 0; i < numItems; i++) {
        item_type kukuItem = make_zero_item();
        memcpy(kukuItem.data(), items.data() + i * itemByteSize, itemByteSize);
        itemsAsKukuType[i] = kukuItem;
    }

    // Receive seed for Kuku hash
    item_type hashSeed;
    sync_wait(chl.recv(hashSeed));

    // Get Kuku hashes
    vector<LocFunc> hashFuncs;
    for (size_t i = 0; i < 3; i++) {
        hashFuncs.emplace_back(mNumBins, hashSeed);
        increment_item(hashSeed);
    }

    // Compute parameters
    size_t tagBitSize = STAT_SEC_PARAM + ceil(log2(mNumBins));
    size_t tagByteSize = (tagBitSize + 7) / 8;
    size_t opprfValueByteSize = tagByteSize + payloadByteSize;

    // Pick random tags and payload shares
    vector<byte> tags(mNumBins * tagByteSize);
    randomBytes(tags.data(), mNumBins * tagByteSize);
    payloadShares.resize(mNumBins * payloadByteSize);
    randomBytes(payloadShares.data(), mNumBins * payloadByteSize);

    // Compute OPPRF inputs and values
    uint32_t opprfInputByteSize = itemByteSize + 1;
    vector<byte> opprfInputs(3 * numItems * opprfInputByteSize);
    vector<byte> opprfValues(3 * numItems * opprfValueByteSize);
    auto opprfInputsPtr = opprfInputs.data();
    auto opprfValuesPtr = opprfValues.data();
    for (size_t i = 0; i < numItems; i++) {
        for (uint32_t j = 0; j < 3; j++) {
            // (3 * i + j)-th input : items[i] || j
            memcpy(opprfInputsPtr, itemsAsKukuType[i].data(), itemByteSize);
            opprfInputsPtr += itemByteSize;
            *(opprfInputsPtr++) = static_cast<byte>((uint8_t)j);

            // (3 * i + j)-th value : tags[loc] || payloads[i] ^ shares[loc]
            auto loc = hashFuncs[j](itemsAsKukuType[i]);
            memcpy(opprfValuesPtr, tags.data() + loc * tagByteSize,
                   tagByteSize);
            opprfValuesPtr += tagByteSize;
            for (size_t k = 0; k < payloadByteSize; k++) {
                *(opprfValuesPtr++) = payloads[i * payloadByteSize + k] ^
                                      payloadShares[loc * payloadByteSize + k];
            }
        }
    }

    if (mTimer != nullptr) {
        mTimer->setTimePoint("cpsisender.computeopprfinputsandvalues");
    }

    mOpprfSenderPtr->send(opprfInputs, opprfValues, opprfInputByteSize,
                          opprfValueByteSize, chl);

    if (mTimer != nullptr) {
        mTimer->setTimePoint("cpsisender.opprf");
    }

    membershipShares = mEqualityCheckPtr->run(tags, tagBitSize, chl);

    if (mTimer != nullptr) {
        mTimer->setTimePoint("cpsisender.equalitycheck");
    }
}

void CPSISender::preprocess(Socket &chl) {
    sync_wait(chl.recv(mNumBins));
    size_t tagBitSize = STAT_SEC_PARAM + ceil(log2(mNumBins));
    mOpprfSenderPtr->sendVectorOLE(chl);
    size_t numOTs = mEqualityCheckPtr->getNumOTs(mNumBins, tagBitSize);
    mEqualityCheckPtr->sendRandomOTs(numOTs, chl);

    if (mTimer != nullptr) {
        mTimer->setTimePoint("cpsisender.preprocess");
    }
}
}  // namespace fuzzypc::protocols