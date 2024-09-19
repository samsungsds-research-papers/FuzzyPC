/* 본 프로그램에 대한 저작권을 포함한 지적재산권은 삼성SDS(주)에 있으며,
삼성SDS(주)가 명시적으로 허용하지 않은 사용, 복사, 변경, 제3자에의 공개,
배포는 엄격히 금지되며, 삼성SDS(주)의 지적재산권 침해에 해당됩니다.
Copyright (c) 2023 Samsung SDS Co., Ltd. All Rights Reserved. Confidential.
All information including the intellectual and technical concepts contained
herein is, and remains the property of Samsung SDS Co. Ltd. Unauthorized use,
dissemination, or reproduction of this material is strictly forbidden unless
prior written permission is obtained from Samsung SDS Co. Ltd.
*/

#pragma once

#include <cryptoTools/Common/BitVector.h>
#include <cryptoTools/Common/Timer.h>
#include <memory>

#include "Defines.h"
#include "EqualityCheck.h"
#include "OpprfReceiver.h"

namespace fuzzypc::protocols {
/**
 * @brief A class for circuit-based private set intersection protocol
 *
 */
class CpsiReceiver {
   public:
    /**
     * @brief Construct a new CpsiReceiver object
     *
     * @param numThreads The number of threads
     */
    CpsiReceiver(size_t numThreads = 1) : mNumThreads(numThreads) {
        mEqualityCheckPtr =
            std::make_shared<EqualityCheck>(false, mNumThreads, mTimer);
        mOpprfReceiverPtr =
            std::make_shared<OPPRFReceiver>(mNumThreads, mTimer);
    }

    /**
     * @brief Set the Timer object
     *
     * @param timer
     */
    void setTimer(oc::Timer *timer) { mTimer = timer; }

    /**
     * @brief Get the number of bins
     *
     * @return size_t
     */
    size_t getNumBins() { return mNumBins; }

    /**
     * @brief Run circuit-based PSI protocol
     *
     * @param[in] items A vector of items
     * @param[in] itemByteSize The byte size of each item
     * @param[in] payloadByteSize  The byte size of each payload
     * @param[out] membershipShares Boolean shares about membership information
     * @param[out] payloadShares Payload shares about sender's input payload
     * @param[out] itemToTableIdx Index map from item to cuckoo table
     * @param[in] chl
     */
    void run(const std::vector<std::byte> &items, uint32_t itemByteSize,
             uint32_t payloadByteSize, oc::BitVector &membershipShares,
             std::vector<std::byte> &payloadShares,
             std::vector<uint64_t> &itemToTableIdx, oc::Socket &chl);

    /**
     * @brief Run preprocess (which is independent to input data)
     *
     * @param numItems
     * @param chl
     */
    void preprocess(size_t numItems, oc::Socket &chl);

   private:
    size_t mNumThreads = 1;

    uint64_t mNumBins = 0, mNumItems = 0;

    std::shared_ptr<EqualityCheck> mEqualityCheckPtr;

    std::shared_ptr<OPPRFReceiver> mOpprfReceiverPtr;

    oc::Timer *mTimer = nullptr;
};
}  // namespace fuzzypc::protocols