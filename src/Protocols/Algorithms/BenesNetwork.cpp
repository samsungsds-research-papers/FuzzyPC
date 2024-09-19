/* 본 프로그램에 대한 저작권을 포함한 지적재산권은 삼성SDS(주)에 있으며,
삼성SDS(주)가 명시적으로 허용하지 않은 사용, 복사, 변경, 제3자에의 공개,
배포는 엄격히 금지되며, 삼성SDS(주)의 지적재산권 침해에 해당됩니다.
Copyright (c) 2023 Samsung SDS Co., Ltd. All Rights Reserved. Confidential.
All information including the intellectual and technical concepts contained
herein is, and remains the property of Samsung SDS Co. Ltd. Unauthorized use,
dissemination, or reproduction of this material is strictly forbidden unless
prior written permission is obtained from Samsung SDS Co. Ltd.
*/

#include "BenesNetwork.h"

#include <stack>

using namespace std;

namespace fuzzypc::algorithms {
void BenesNetwork::buildConnectionInner(size_t numInputs, size_t colIdxStart,
                                        size_t colIdxEnd, size_t inputOffset,
                                        size_t switchOffset) {
    size_t numCols = colIdxEnd - colIdxStart;
    if (numInputs == 2) {
        Switch s = {inputOffset, inputOffset + 1, inputOffset, inputOffset + 1,
                    0};
        if (numCols == 1) {
            switches_[colIdxStart][switchOffset] = s;
        } else if (numCols == 3) {
            switches_[colIdxStart + 1][switchOffset] = s;
        } else {
            throw invalid_argument(
                "numCols should be 1 or 3 if numInputs is 2");
        }
        return;
    }
    if (numInputs == 3) {
        Switch s1 = {inputOffset, inputOffset + 1, inputOffset, inputOffset + 1,
                     0};
        Switch s2 = {inputOffset + 1, inputOffset + 2, inputOffset + 1,
                     inputOffset + 2, 0};
        Switch s3 = {inputOffset, inputOffset + 1, inputOffset, inputOffset + 1,
                     0};
        switches_[colIdxStart][switchOffset] = s1;
        switches_[colIdxStart + 1][switchOffset] = s2;
        switches_[colIdxStart + 2][switchOffset] = s3;
        return;
    }
    size_t upperPermSize = numInputs / 2;
    size_t lowerPermSize = numInputs - upperPermSize;

    for (size_t i = 0; i < upperPermSize; i++) {
        Switch s1 = {inputOffset + 2 * i, inputOffset + 2 * i + 1,
                     inputOffset + i, inputOffset + i + upperPermSize, 0};
        Switch s2 = {inputOffset + i, inputOffset + i + upperPermSize,
                     inputOffset + 2 * i, inputOffset + 2 * i + 1, 0};
        switches_[colIdxStart][switchOffset + i] = s1;
        switches_[colIdxEnd - 1][switchOffset + i] = s2;
    }
    buildConnectionInner(upperPermSize, colIdxStart + 1, colIdxEnd - 1,
                         inputOffset, switchOffset);
    buildConnectionInner(lowerPermSize, colIdxStart + 1, colIdxEnd - 1,
                         inputOffset + upperPermSize,
                         switchOffset + (upperPermSize / 2));
}

void BenesNetwork::routingInner(IntegerPermutation &perm,
                                IntegerPermutation &permInv, size_t numInputs,
                                size_t colIdxStart, size_t colIdxEnd,
                                size_t inputOffset, size_t switchOffset) {
    size_t numCols = colIdxEnd - colIdxStart;
    if (numInputs == 2) {
        if (numCols == 1) {
            switches_[colIdxStart][switchOffset].isSwitched = perm.get(0) != 0;
        } else if (numCols == 3) {
            switches_[colIdxStart + 1][switchOffset].isSwitched =
                perm.get(0) != 0;
        } else {
            throw invalid_argument(
                "numCols should be 1 or 3 if numInputs is 2");
        }
        return;
    }
    if (numInputs == 3) {
        switches_[colIdxStart][switchOffset].isSwitched = perm.get(0) == 2;
        if (perm.get(0) == 0) {
            switches_[colIdxStart + 1][switchOffset].isSwitched =
                perm.get(1) != 1;
        } else if (perm.get(0) == 1) {
            switches_[colIdxStart + 1][switchOffset].isSwitched =
                perm.get(1) != 0;
            switches_[colIdxStart + 2][switchOffset].isSwitched = 1;
        } else if (perm.get(0) == 2) {
            switches_[colIdxStart + 1][switchOffset].isSwitched = 1;
            switches_[colIdxStart + 2][switchOffset].isSwitched =
                perm.get(1) != 0;
        }
        return;
    }
    size_t upperPermSize = numInputs / 2;
    size_t lowerPermSize = numInputs - upperPermSize;

    IntegerPermutation perm1(0, upperPermSize - 1);
    IntegerPermutation perm1Inv(0, upperPermSize - 1);
    IntegerPermutation perm2(0, lowerPermSize - 1);
    IntegerPermutation perm2Inv(0, lowerPermSize - 1);

    vector<bool> visited(numInputs, false);
    vector<bool> forward(numInputs);
    vector<bool> backward(numInputs);

    vector<size_t> permOfIdx(numInputs);
    vector<size_t> permInvOfIdx(numInputs);
    for (size_t i = 0; i < numInputs; i++) {
        permOfIdx[i] = perm.get(i);
        permInvOfIdx[i] = permInv.get(i);
    }

    auto loop = [&](size_t idx, bool upper) {
        size_t x, xprime, y, yprime;
        x = idx;
        while (1) {
            forward[x] = upper;
            visited[x] = true;
            xprime = permOfIdx[x];
            backward[xprime] = upper;
            yprime = xprime ^ 1;
            backward[yprime] = upper ^ 1;
            y = permInvOfIdx[yprime];
            forward[y] = upper ^ 1;
            visited[y] = true;
            if (!visited[y ^ 1]) {
                x = y ^ 1;
            } else {
                break;
            }
        }
    };

    size_t xprime, y, yprime, z;
    if (numInputs % 2 == 1) {
        forward[numInputs - 1] = false;  // go to lower
        visited[numInputs - 1] = true;
        xprime = permOfIdx[numInputs - 1];
        backward[xprime] = false;  // go to lower
        if (xprime != (numInputs - 1)) {
            yprime = xprime ^ 1;
            backward[yprime] = true;  // go to upper
            y = permInvOfIdx[yprime];
            forward[y] = true;  // go to upper
            visited[y] = true;
            z = permInvOfIdx[numInputs - 1];
            forward[z] = false;  // go to lower
            visited[z] = true;
            if (!visited[y ^ 1]) {
                loop(y ^ 1, false);
            }
        }
    }

    for (size_t i = 0; i < numInputs; i++) {
        if (!visited[i]) {
            loop(i, true);
        }
    }

    for (size_t i = 0; i < numInputs; i++) {
        auto iprime = permOfIdx[i];
        if (forward[i]) {
            perm1.set(i / 2, iprime / 2);
            perm1Inv.set(iprime / 2, i / 2);
        } else {
            perm2.set(i / 2, iprime / 2);
            perm2Inv.set(iprime / 2, i / 2);
        }
    }

    for (size_t i = 0; i < upperPermSize; i++) {
        switches_[colIdxStart][switchOffset + i].isSwitched =
            forward[2 * i + 1];
        switches_[colIdxEnd - 1][switchOffset + i].isSwitched =
            backward[2 * i + 1];
    }

    routingInner(perm1, perm1Inv, upperPermSize, colIdxStart + 1, colIdxEnd - 1,
                 inputOffset, switchOffset);
    routingInner(perm2, perm2Inv, lowerPermSize, colIdxStart + 1, colIdxEnd - 1,
                 inputOffset + upperPermSize,
                 switchOffset + (upperPermSize / 2));
}
}  // namespace pprl::algorithms