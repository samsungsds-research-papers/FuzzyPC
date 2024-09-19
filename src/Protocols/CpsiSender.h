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

#include <memory>

#include "Defines.h"
#include "EqualityCheck.h"
#include "OpprfSender.h"

namespace fuzzypc::protocols {
/**
 * @brief
 *
 */
class CPSISender {
    /**
     * @brief
     *
     */
   public:
    CPSISender(size_t numThreads = 1, oc::Timer *timer = nullptr)
        : mNumThreads(numThreads), mTimer(timer) {
        mEqualityCheckPtr =
            std::make_shared<EqualityCheck>(true, mNumThreads, mTimer);
        mOpprfSenderPtr = std::make_shared<OPPRFSender>(mNumThreads, mTimer);
    }

    /**
     * @brief Run circuit-based PSI protocl
     *
     * @param items
     * @param itemByteSize
     * @param payloads
     * @param payloadByteSize
     * @param membershipShares
     * @param payloadShares
     * @param socket
     *
     * @warning make sure that there is no duplicated items
     */
    void run(const std::vector<std::byte> &items, const uint32_t itemByteSize,
             const std::vector<std::byte> &payloads,
             const uint32_t payloadByteSize, oc::BitVector &membershipShares,
             std::vector<std::byte> &payloadShares, oc::Socket &socket);

    /**
     * @brief
     *
     * @param socket
     */
    void preprocess(oc::Socket &socket);

    /**
     * @brief Get the number of bins
     *
     * @return size_t
     */
    size_t getNumBins() { return mNumBins; }

   private:
    size_t mNumThreads = 1;

    uint64_t mTableSizePreDefined = 0, mNumBins = 0;

    std::shared_ptr<EqualityCheck> mEqualityCheckPtr;

    std::shared_ptr<OPPRFSender> mOpprfSenderPtr;

    oc::Timer *mTimer = nullptr;
};
}  // namespace fuzzypc::protocols