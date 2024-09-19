/* 본 프로그램에 대한 저작권을 포함한 지적재산권은 삼성SDS(주)에 있으며,
삼성SDS(주)가 명시적으로 허용하지 않은 사용, 복사, 변경, 제3자에의 공개,
배포는 엄격히 금지되며, 삼성SDS(주)의 지적재산권 침해에 해당됩니다.
Copyright (c) 2023 Samsung SDS Co., Ltd. All Rights Reserved. Confidential.
All information including the intellectual and technical concepts contained
herein is, and remains the property of Samsung SDS Co. Ltd. Unauthorized use,
dissemination, or reproduction of this material is strictly forbidden unless
prior written permission is obtained from Samsung SDS Co. Ltd.
*/

#include "CpsiReceiver.h"

#include <kuku/kuku.h>
#include <unordered_set>

#include "Common/Random.h"

using namespace std;
using namespace kuku;
using namespace coproto;
using namespace oc;
using namespace fuzzypc::common;

namespace fuzzypc::protocols {
void CpsiReceiver::run(const vector<byte> &items, const uint32_t itemByteSize,
                       const uint32_t payloadByteSize,
                       BitVector &membershipShares, vector<byte> &payloadShares,
                       vector<uint64_t> &itemToTableIdx, Socket &chl) {
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

    // Pick seed for cuckoo hash and share it
    item_type kukuSeed = make_item(random64(), random64());
    auto kukuSeedCopy = kukuSeed;
    sync_wait(chl.send(move(kukuSeedCopy)));

    // Build cuckoo table
    KukuTable kukuTable(mNumBins, 0, 3, kukuSeed, 500,
                        make_item(UINT64_MAX, UINT64_MAX));
    for (size_t i = 0; i < numItems; i++) {
        if (!kukuTable.insert(itemsAsKukuType[i])) {
            throw invalid_argument("Kuku table insert failed");
        }
    }

    if (mTimer != nullptr) {
        mTimer->setTimePoint("CpsiReceiver.run.buildkukutable");
    }

    // Get index map between item and cuckoo table
    itemToTableIdx.resize(numItems);
    unordered_map<size_t, size_t> tableToItemIdx;
    for (size_t i = 0; i < numItems; i++) {
        auto queryResult = kukuTable.query(itemsAsKukuType[i]);
        tableToItemIdx[queryResult.location()] = i;
        itemToTableIdx[i] = queryResult.location();
    }

    // Compute (items[i] || hashIndex)
    size_t opprfInputByteSize = itemByteSize + 1;
    vector<byte> opprfInputs(numItems * opprfInputByteSize);
    for (size_t i = 0; i < numItems; i++) {
        auto queryResult = kukuTable.query(itemsAsKukuType[i]);
        memcpy(opprfInputs.data() + i * opprfInputByteSize,
               items.data() + i * itemByteSize, itemByteSize);
        opprfInputs[i * opprfInputByteSize + itemByteSize] =
            static_cast<byte>((uint8_t)queryResult.loc_func_index());
    }

    if (mTimer != nullptr) {
        mTimer->setTimePoint("CpsiReceiver.run.computeopprfinputs");
    }

    // Compute parameters
    size_t tagBitSize = STAT_SEC_PARAM + ceil(log2(mNumBins));
    size_t tagByteSize = (tagBitSize + 7) / 8;
    size_t opprfOutputByteSize = tagByteSize + payloadByteSize;

    // Run oblivious PRF protocol
    auto opprfResults = mOpprfReceiverPtr->recv(opprfInputs, opprfInputByteSize,
                                                opprfOutputByteSize, chl);

    if (mTimer != nullptr) {
        mTimer->setTimePoint("CpsiReceiver.run.opprf");
    }

    // Pick random tags
    vector<byte> tags(mNumBins * tagByteSize), dummyTag(tagByteSize);
    PRNG prng(toBlock(random64(), random64()));
    prng.get(tags.data(), tags.size());
    prng.get(dummyTag.data(), dummyTag.size());

    for (size_t i = 0; i < mNumBins; i++) {
        if (kukuTable.is_empty(i)) {
            // If i-th bin is empty, put (random) dummy tag
            memcpy(tags.data() + i * tagByteSize, dummyTag.data(), tagByteSize);
        } else {
            auto itemIdx = tableToItemIdx[i];
            memcpy(tags.data() + i * tagByteSize,
                   opprfResults.data() + itemIdx * opprfOutputByteSize,
                   tagByteSize);
        }
    }

    if (mTimer != nullptr) {
        mTimer->setTimePoint("CpsiReceiver.run.computetags");
    }

    // Run Equality Check protocol
    membershipShares = mEqualityCheckPtr->run(tags, tagBitSize, chl);

    if (mTimer != nullptr) {
        mTimer->setTimePoint("CpsiReceiver.run.equalitycheck");
    }

    // Get payload shares
    vector<byte> dummyPayload(payloadByteSize);
    payloadShares.resize(mNumBins * payloadByteSize);
    for (size_t i = 0; i < mNumBins; i++) {
        if (kukuTable.is_empty(i)) {
            prng.get(dummyPayload.data(), dummyPayload.size());
            memcpy(payloadShares.data() + i * payloadByteSize,
                   dummyPayload.data(), payloadByteSize);
        } else {
            auto itemIdx = tableToItemIdx[i];
            memcpy(payloadShares.data() + i * payloadByteSize,
                   opprfResults.data() + itemIdx * opprfOutputByteSize +
                       tagByteSize,
                   payloadByteSize);
        }
    }

    if (mTimer != nullptr) {
        mTimer->setTimePoint("CpsiReceiver.run.opprfresultstoshares");
    }
}

void CpsiReceiver::preprocess(size_t numItems, Socket &chl) {
    mNumItems = numItems;
    mNumBins = ceil(numItems * OKVS_EXPANSION);
    sync_wait(chl.send(move(mNumBins)));
    size_t tagBitSize = STAT_SEC_PARAM + ceil(log2(mNumBins));
    mOpprfReceiverPtr->recvVectorOLE(numItems, chl);
    size_t numOTs = mEqualityCheckPtr->getNumOTs(mNumBins, tagBitSize);
    mEqualityCheckPtr->recvRandomOTs(numOTs, chl);

    if (mTimer != nullptr) {
        mTimer->setTimePoint("cpsireceiver.preprocess");
    }
}
}  // namespace fuzzypc::protocols