/* 본 프로그램에 대한 저작권을 포함한 지적재산권은 삼성SDS(주)에 있으며,
삼성SDS(주)가 명시적으로 허용하지 않은 사용, 복사, 변경, 제3자에의 공개,
배포는 엄격히 금지되며, 삼성SDS(주)의 지적재산권 침해에 해당됩니다.
Copyright (c) 2023 Samsung SDS Co., Ltd. All Rights Reserved. Confidential.
All information including the intellectual and technical concepts contained
herein is, and remains the property of Samsung SDS Co. Ltd. Unauthorized use,
dissemination, or reproduction of this material is strictly forbidden unless
prior written permission is obtained from Samsung SDS Co. Ltd.
*/

#include "FuzzyPc.h"

#include <xxh3.h>
#include "Common/Utils.h"

using namespace std;
using namespace osuCrypto;
using namespace fuzzypc::common;

namespace fuzzypc {
vector<byte> FuzzyPc::dedupAndHash(const vector<string>& column,
                                   const size_t hashByteSize,
                                   vector<size_t>& deduplicatedToRows) {
    // clear memory
    deduplicatedToRows.clear();

    // deduplicate
    vector<string> deduplicated;
    deduplication(column, deduplicated, deduplicatedToRows);

    // compute hash
    vector<byte> result(deduplicated.size() * hashByteSize);
    for (size_t i = 0; i < deduplicated.size(); i++) {
        auto h = XXH3_128bits(deduplicated[i].data(), deduplicated[i].size());
        block b = toBlock(h.high64, h.low64);
        memcpy(result.data() + i * hashByteSize, &b, hashByteSize);
    }

    return result;
}

vector<byte> FuzzyPc::dedupAndHash(const vector<uint64_t>& column,
                                   const size_t hashByteSize,
                                   vector<size_t>& deduplicatedToRows) {
    // clear memory
    deduplicatedToRows.clear();

    // deduplicate
    vector<uint64_t> deduplicated;
    deduplication(column, deduplicated, deduplicatedToRows);

    // compute hash
    vector<byte> result(sizeof(uint64_t) * hashByteSize);
    for (size_t i = 0; i < deduplicated.size(); i++) {
        auto h = XXH3_128bits(&deduplicated[i], sizeof(uint64_t));
        block b = toBlock(h.high64, h.low64);
        memcpy(result.data() + i * hashByteSize, &b, hashByteSize);
    }

    return result;
}
}  // namespace fuzzypc