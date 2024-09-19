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
#include <libOTe/Tools/Coproto.h>

#include "Defines.h"
#include "FuzzyPc.h"
#include "Protocols/CpsiReceiver.h"
#include "Protocols/PnsSender.h"
#include "Protocols/SecretSharedOt.h"

namespace fuzzypc {
/**
 * @brief
 *
 */
class FuzzyPcReceiver : public FuzzyPc {
   public:
    /**
     * @brief Construct a new FuzzyPcReceiver object
     *
     * @param numRecords
     * @param numBins
     * @param itemByteSize
     * @param idByteSize
     * @param numThreads
     * @param timer
     */
    FuzzyPcReceiver(uint64_t numRecords, uint64_t numBins, uint32_t itemByteSize,
                 uint32_t idByteSize, size_t numThreads = 1,
                 oc::Timer *timer = nullptr)
        : mNumRows(numRecords),
          mNumBins(numBins),
          mHashByteSize(itemByteSize),
          mPayloadByteSize(idByteSize),
          mNumThreads(numThreads),
          mTimer(timer){};

    void runCpsiAndPns(const std::vector<std::byte> &deduplicated,
                       const std::vector<uint64_t> &deduplicatedToInput,
                       oc::BitVector &membershipShares,
                       std::vector<std::byte> &payloadShares, size_t featureIdx,
                       oc::Socket &socket);

    /**
     * @brief
     *
     * @param input
     * @param socket
     * @return std::vector<std::byte>
     */
    std::vector<std::byte> run(
        const std::vector<std::vector<std::string>> &input, oc::Socket &socket);

    /**
     * @brief
     *
     * @param input
     * @param socket
     * @return std::vector<std::byte>
     */
    std::vector<std::byte> run(const std::vector<std::vector<uint64_t>> &input,
                               oc::Socket &socket);

    /**
     * @brief Get the Global Idx object
     *
     * @return std::vector<uint64_t>
     */
    std::vector<uint64_t> getGlobalIndex() { return mGlobalIndex; }

    /**
     * @brief Get the Item Byte Size object
     *
     * @return uint32_t
     */
    uint32_t getItemByteSize() { return mHashByteSize; };

    /**
     * @brief Get the Payload Byte Size object
     *
     * @return uint32_t
     */
    uint32_t getPayloadByteSize() { return mPayloadByteSize; };

    /**
     * @brief
     *
     * @param maxNumItems
     * @param numFeatures
     * @param idByteSize
     * @param chl
     */
    void preprocess(size_t maxNumItems, size_t numFeatures, size_t idByteSize,
                    oc::Socket &chl);

   private:
    size_t mNumThreads = 1, mNumRows = 0, mNumBins = 0, mHashByteSize = 0,
           mPayloadByteSize = 0;

    std::vector<uint64_t> mGlobalIndex;

    oc::PRNG mPrng{oc::CCBlock};

    std::vector<std::shared_ptr<fuzzypc::protocols::CpsiReceiver>>
        mCpsiReceiverPtrs;

    std::vector<std::shared_ptr<fuzzypc::protocols::PnsSender>> mPnsSenderPtrs;

    std::shared_ptr<fuzzypc::protocols::SecretSharedOT> mSecretSharedOtPtr;

    oc::Timer *mTimer = nullptr;
};
}  // namespace fuzzypc