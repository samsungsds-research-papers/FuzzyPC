/* 본 프로그램에 대한 저작권을 포함한 지적재산권은 삼성SDS(주)에 있으며,
삼성SDS(주)가 명시적으로 허용하지 않은 사용, 복사, 변경, 제3자에의 공개,
배포는 엄격히 금지되며, 삼성SDS(주)의 지적재산권 침해에 해당됩니다.
Copyright (c) 2023 Samsung SDS Co., Ltd. All Rights Reserved. Confidential.
All information including the intellectual and technical concepts contained
herein is, and remains the property of Samsung SDS Co. Ltd. Unauthorized use,
dissemination, or reproduction of this material is strictly forbidden unless
prior written permission is obtained from Samsung SDS Co. Ltd.
*/

#include "IntegerPermutation.h"

#include <algorithm>
#include <numeric>
#include <stdexcept>

using namespace std;

namespace fuzzypc::structures {
IntegerPermutation::IntegerPermutation(size_t minElement, size_t maxElement)
    : mMinElement(minElement), mMaxElement(maxElement) {
    if (minElement > maxElement) {
        throw invalid_argument("minElement is larger than maxElement at " +
                               string(__FILE__));
    }
    mData.resize(maxElement - minElement + 1);
    iota(mData.begin(), mData.end(), minElement);
}

size_t IntegerPermutation::get(size_t from) const {
    if (from < mMinElement || from > mMaxElement) {
        throw invalid_argument("from is out of range at " + string(__FILE__));
    }
    return mData[from - mMinElement];
}

void IntegerPermutation::set(size_t from, size_t to) {
    if (from < mMinElement || from > mMaxElement) {
        throw invalid_argument("from is out of range at " + string(__FILE__));
    }
    if (to < mMinElement || to > mMaxElement) {
        throw invalid_argument("to is out of range at " + string(__FILE__));
    }
    mData[from - mMinElement] = to;
}

IntegerPermutation IntegerPermutation::inverse() const {
    IntegerPermutation result(mMinElement, mMaxElement);

    for (size_t i = mMinElement; i <= mMaxElement; i++) {
        result.mData[this->mData[i - mMinElement] - mMinElement] = i;
    }

    return result;
}

void IntegerPermutation::randomShuffle() {
    random_shuffle(mData.begin(), mData.end());
}
}  // namespace pprl::structures