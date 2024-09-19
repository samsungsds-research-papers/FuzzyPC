/* 본 프로그램에 대한 저작권을 포함한 지적재산권은 삼성SDS(주)에 있으며,
삼성SDS(주)가 명시적으로 허용하지 않은 사용, 복사, 변경, 제3자에의 공개,
배포는 엄격히 금지되며, 삼성SDS(주)의 지적재산권 침해에 해당됩니다.
Copyright (c) 2023 Samsung SDS Co., Ltd. All Rights Reserved. Confidential.
All information including the intellectual and technical concepts contained
herein is, and remains the property of Samsung SDS Co. Ltd. Unauthorized use,
dissemination, or reproduction of this material is strictly forbidden unless
prior written permission is obtained from Samsung SDS Co. Ltd.
*/

#include "Utils.h"
#include "Random.h"

#include <algorithm>
#include <cassert>
#include <fstream>
#include <set>
#include <sstream>
#include <unordered_set>

#include "xxhash.h"

const int SOUNDEX_CODE_LENGTH = 4;  // length of soundex code

using namespace std;
using namespace osuCrypto;

namespace fuzzypc::common {
string remainAlphaAndDigit(string s) {
    string res = "";
    for (auto ch : s) {
        if (isalpha(ch)) {
            res += tolower(ch);
        }
        if (isdigit(ch)) {
            res += ch;
        }
    }
    return res;
}

map<string, vector<string>> random(map<string, vector<string>> &table,
                                   size_t numRows, bool firstRows) {
    size_t numRowsInTable = 0;
    for (auto column : table) {
        if (numRowsInTable == 0) {
            numRowsInTable = column.second.size();
        } else {
            if (numRowsInTable != column.second.size()) {
                throw invalid_argument(
                    "number of rows in each column is different");
            }
        }
    }
    vector<size_t> indices;
    for (size_t i = 0; i < numRowsInTable; i++) {
        indices.push_back(i);
    }
    if (!firstRows) random_shuffle(indices.begin(), indices.end());
    map<string, vector<string>> result;
    for (auto column : table) {
        for (size_t i = 0; i < numRows; i++) {
            result[column.first].push_back(column.second[indices[i]]);
        }
    }
    return result;
}

vector<byte> hash(const vector<string> &column, const uint32_t hashByteLen,
                  const vector<byte> &dummy) {
    return vector<byte>();
}

vector<string> concatenate(map<string, vector<string>> &table,
                           vector<string> features, bool removeNonAlphanumeric,
                           bool spreadNan) {
    vector<string> result = table[features[0]];
    for (size_t i = 0; i < result.size(); i++) {
        if (removeNonAlphanumeric) result[i] = remainAlphaAndDigit(result[i]);
    }
    for (size_t i = 1; i < features.size(); i++) {
        auto column = table[features[i]];
        if (column.size() != result.size()) {
            throw invalid_argument(
                "number of rows in each column is different");
        }
        for (size_t j = 0; j < column.size(); j++) {
            if (spreadNan) {
                if (result[j] != "" && column[j] != "") {
                    if (removeNonAlphanumeric)
                        result[j] += remainAlphaAndDigit(column[j]);
                    else
                        result[j] += column[j];
                } else {
                    result[j] = "";
                }
            } else {
                if (removeNonAlphanumeric)
                    result[j] += remainAlphaAndDigit(column[j]);
                else
                    result[j] += column[j];
            }
        }
    }
    for (size_t i = 0; i < result.size(); i++) {
        result[i].erase(remove(result[i].begin(), result[i].end(), ' '),
                        result[i].end());
    }
    return result;
}

vector<vector<string>> concatenate(map<string, vector<string>> &table,
                                   vector<vector<string>> features,
                                   bool removeNonAlphanumeric, bool spreadNan) {
    size_t numRowsInTable = 0;
    for (auto column : table) {
        if (numRowsInTable == 0) {
            numRowsInTable = column.second.size();
        } else {
            if (numRowsInTable != column.second.size()) {
                throw invalid_argument(
                    "number of rows in each column is different");
            }
        }
    }
    vector<vector<string>> result(numRowsInTable);
    for (size_t i = 0; i < features.size(); i++) {
        auto tmp =
            concatenate(table, features[i], removeNonAlphanumeric, spreadNan);
        for (size_t j = 0; j < tmp.size(); j++) {
            result[j].push_back(tmp[j]);
        }
    }
    return result;
}

void deduplication(const vector<string> &column, vector<string> &deduplicated,
                   vector<size_t> &deduplicatedToRows) {
    // clear memory
    deduplicated.clear();
    deduplicatedToRows.clear();

    // find duplicated items using unordered_map
    unordered_map<string, vector<size_t>> mapFromStringToIndex;
    for (size_t i = 0; i < column.size(); i++) {
        mapFromStringToIndex[column[i]].push_back(i);
    }

    // randomly select one of duplicated items
    vector<bool> checked(column.size(), false);
    for (size_t i = 0; i < column.size(); i++) {
        if (!checked[i] && column[i] != "") {
            vector<size_t> indices = mapFromStringToIndex[column[i]];
            if (indices.size() > 1) {
                size_t selected = random64() % indices.size();
                deduplicated.push_back(column[indices[selected]]);
                deduplicatedToRows.push_back(indices[selected]);
                for (size_t j = 0; j < indices.size(); j++) {
                    checked[indices[j]] = true;
                }
            } else {  // no duplication case
                deduplicated.push_back(column[i]);
                deduplicatedToRows.push_back(i);
                checked[i] = true;
            }
        }
    }
}

void deduplication(const vector<uint64_t> &column,
                   vector<uint64_t> &deduplicated,
                   vector<size_t> &indexMapFromDeduplicated) {
    // clear memory
    deduplicated.clear();
    indexMapFromDeduplicated.clear();

    // find duplicated items using unordered_map
    unordered_map<uint64_t, vector<size_t>> mapFromStringToIndex;
    for (size_t i = 0; i < column.size(); i++) {
        mapFromStringToIndex[column[i]].push_back(i);
    }

    // randomly select one of duplicated items
    vector<bool> checked(column.size(), false);
    for (size_t i = 0; i < column.size(); i++) {
        if (!checked[i] && column[i] != 0) {
            vector<size_t> indices = mapFromStringToIndex[column[i]];
            if (indices.size() > 1) {
                size_t selected = random64() % indices.size();
                deduplicated.push_back(column[indices[selected]]);
                indexMapFromDeduplicated.push_back(indices[selected]);
                for (size_t j = 0; j < indices.size(); j++) {
                    checked[indices[j]] = true;
                }
            } else {  // no duplication case
                deduplicated.push_back(column[i]);
                indexMapFromDeduplicated.push_back(i);
                checked[i] = true;
            }
        }
    }
}

void accuracy(unordered_map<size_t, size_t> &trueLink,
              unordered_map<size_t, size_t> &link, size_t N) {
    size_t tp = 0, fn = 0, fp = 0, tn = 0;
    for (size_t i = 0; i < N; i++) {
        auto iter1 = trueLink.find(i);
        if (iter1 != trueLink.end()) {
            auto iter2 = link.find(i);
            if (iter2 != link.end()) {
                if (trueLink[i] == link[i]) {
                    tp++;
                } else {
                    fn++;
                }
            } else {
                fn++;
            }
        } else {
            auto iter2 = link.find(i);
            if (iter2 != link.end()) {
                fp++;
            } else {
                tn++;
            }
        }
    }
    cout << "\t* fn / tp / fp / tn : ";
    cout << fn << " / " << tp << " / " << fp << " / " << tn << endl;

    double acc = double(tp + tn) / double(tp + fp + fn + tn);
    double ppv = double(tp) / double(tp + fp);
    double recall = double(tp) / double(tp + fn);
    double f1 = double(2 * recall * ppv) / double(recall + ppv);

    cout << "\t* acc : " << acc << endl;
    cout << "\t* precision : " << ppv << endl;
    cout << "\t* recall : " << recall << endl;
    cout << "\t* f1 : " << f1 << endl;
}

char soundexEncodedChar(char c) {
    if (c == 'A' || c == 'E' || c == 'I' || c == 'O' || c == 'U' || c == 'H' ||
        c == 'W' || c == 'Y') {
        return '0';
    } else if (c == 'B' || c == 'F' || c == 'P' || c == 'V') {
        return '1';
    } else if (c == 'C' || c == 'G' || c == 'J' || c == 'K' || c == 'Q' ||
               c == 'S' || c == 'X' || c == 'Z') {
        return '2';
    } else if (c == 'D' || c == 'T') {
        return '3';
    } else if (c == 'M' || c == 'N') {
        return '4';
    } else if (c == 'L') {
        return '5';
    } else {
        return '6';
    }
}

string soundex(const string &input) {
    string result = "";
    if (isalpha(input[0])) {
        result += toupper(input[0]);
        for (size_t i = 1; i < input.length(); i++) {
            if (isalpha(input[i])) {
                result += soundexEncodedChar(toupper(input[i]));
            }
        }
        for (int i = 0; i < result.length(); i++) {
            if (i > 0 && result[i] == result[i - 1]) {
                result.erase(i, 1);  // delete duplicates
            } else if (result[i] == '0') {
                result.erase(i, 1);  // delete zeros
            }
        }
        if (result.length() < SOUNDEX_CODE_LENGTH) {
            for (size_t i = 0; i < SOUNDEX_CODE_LENGTH - result.length(); i++) {
                result += '0';
            }
        } else if (result.length() > SOUNDEX_CODE_LENGTH) {
            result = result.substr(0, SOUNDEX_CODE_LENGTH);
        }
    }
    return result;
}

size_t equality(const vector<string> &input1, const vector<string> &input2) {
    if (input1.size() != input2.size()) {
        throw invalid_argument("input1 and input2 have different size");
    }
    size_t result = 0;
    for (size_t i = 0; i < input1.size(); i++) {
        if (input1[i].size() != 0 && input1[i].compare(input2[i]) == 0) {
            result++;
        }
    }
    return result;
}

unordered_map<size_t, size_t> equality(const vector<vector<string>> &input1,
                                       const vector<vector<string>> &input2) {
    size_t numFeatures = input1[0].size();

    vector<unordered_map<string, size_t>> map2(numFeatures);
    for (size_t i = 0; i < input2.size(); i++) {
        for (size_t j = 0; j < numFeatures; j++) {
            if (input2[i][j] != "") {
                map2[j][input2[i][j]] = i;
            }
        }
    }

    unordered_map<size_t, size_t> result;
    for (size_t i = 0; i < input1.size(); i++) {
        unordered_map<size_t, size_t> count1;
        for (size_t j = 0; j < numFeatures; j++) {
            if (map2[j].find(input1[i][j]) != map2[j].end()) {
                if (count1.find(map2[j][input1[i][j]]) != count1.end()) {
                    count1[map2[j][input1[i][j]]]++;
                } else {
                    count1[map2[j][input1[i][j]]] = 1;
                }
            }
        }
        size_t max = 0;
        for (auto c : count1) {
            if (c.second > max) {
                max = c.second;
            }
        }
        if (max > 0) {
            vector<size_t> candidates;
            for (auto c : count1) {
                if (c.second == max) {
                    candidates.push_back(c.first);
                }
            }
            random_shuffle(candidates.begin(), candidates.end());
            result[i] = candidates[0];
        }
    }
    return result;
}

unordered_map<size_t, size_t> equality(const vector<vector<uint64_t>> &input1,
                                       const vector<vector<uint64_t>> &input2) {
    size_t numFeatures = input1[0].size();

    vector<unordered_map<uint64_t, size_t>> map2(numFeatures);
    for (size_t i = 0; i < input2.size(); i++) {
        for (size_t j = 0; j < numFeatures; j++) {
            if (input2[i][j] != 0) {
                map2[j][input2[i][j]] = i;
            }
        }
    }

    unordered_map<size_t, size_t> result;
    for (size_t i = 0; i < input1.size(); i++) {
        unordered_map<size_t, size_t> count1;
        for (size_t j = 0; j < numFeatures; j++) {
            if (map2[j].find(input1[i][j]) != map2[j].end()) {
                if (count1.find(map2[j][input1[i][j]]) != count1.end()) {
                    count1[map2[j][input1[i][j]]]++;
                } else {
                    count1[map2[j][input1[i][j]]] = 1;
                }
            }
        }
        size_t max = 0;
        for (auto c : count1) {
            if (c.second > max) {
                max = c.second;
            }
        }
        if (max > 0) {
            vector<size_t> candidates;
            for (auto c : count1) {
                if (c.second == max) {
                    candidates.push_back(c.first);
                }
            }
            random_shuffle(candidates.begin(), candidates.end());
            result[i] = candidates[0];
        }
    }
    return result;
}

unordered_map<size_t, size_t> threshold1(
    const vector<vector<uint64_t>> &input1,
    const vector<vector<uint64_t>> &input2) {
    size_t numFeatures = input1[0].size();

    unordered_map<size_t, size_t> result;
    for (size_t i = 0; i < numFeatures; i++) {
        unordered_map<uint64_t, size_t> map2;
        for (size_t j = 0; j < input2.size(); j++) {
            if (input2[j][i] != 0) {
                map2[input2[j][i]] = j;
            }
        }
        for (size_t j = 0; j < input1.size(); j++) {
            if (map2.find(input1[j][i]) != map2.end()) {
                result[j] = map2[input1[j][i]];
            }
        }
    }
    return result;
}

void preprocessColumn(const vector<byte> &rawColumnInByte,
                      const uint32_t itemByteSize,
                      vector<byte> &processedColumInByte,
                      vector<uint64_t> &idxItemToRow,
                      const vector<byte> &dummy) {
    assert(rawColumnInByte.size() % itemByteSize == 0);
    uint32_t numRows = rawColumnInByte.size() / itemByteSize;

    vector<block> rawItemsInBlock(numRows);
    for (size_t i = 0; i < numRows; i++) {
        block tmp = ZeroBlock;
        memcpy(&tmp, rawColumnInByte.data() + i * itemByteSize, itemByteSize);
        rawItemsInBlock[i] = tmp;
    }

    unordered_map<block, uint64_t> idxItemToRowTmp;
    for (int i = numRows - 1; i >= 0; i--) {
        idxItemToRowTmp[rawItemsInBlock[i]] = i;
    }

    unordered_set<block> deduplicate(rawItemsInBlock.begin(),
                                     rawItemsInBlock.end());
    block dummy_block = ZeroBlock;
    memcpy(&dummy_block, dummy.data(), itemByteSize);
    deduplicate.erase(dummy_block);
    numRows = deduplicate.size();

    for (auto &elt : deduplicate) {
        idxItemToRow.push_back(idxItemToRowTmp[elt]);
    }

    processedColumInByte.resize(itemByteSize * numRows);
    for (size_t i = 0; i < numRows; i++) {
        memcpy(processedColumInByte.data() + i * itemByteSize,
               rawColumnInByte.data() + idxItemToRow[i] * itemByteSize,
               itemByteSize);
    }
}

map<string, vector<string>> readTable(const string &fileName,
                                      const char delimiter,
                                      const bool hasHeader,
                                      const uint32_t maxNumRows) {
    ifstream fin;
    fin.open(fileName);
    if (!fin.is_open()) {
        throw invalid_argument("Fail to open file");
    }
    vector<string> lineVec;
    string line;
    string item;

    map<string, vector<string>> table;
    vector<string> header;
    getline(fin, line);
    istringstream ss(line);

    uint32_t numCols;

    if (hasHeader) {
        while (getline(ss, item, delimiter)) {
            item.erase(remove(item.begin(), item.end(), '\"'), item.end());
            header.push_back(item);
        }
        for (size_t i = 0; i < header.size(); i++) {
            table[header[i]] = vector<string>();
        }
        numCols = header.size();
    } else {
        while (getline(ss, item, delimiter)) {
            item.erase(remove(item.begin(), item.end(), '\"'), item.end());
            lineVec.push_back(item);
        }
        for (size_t i = 0; i < lineVec.size(); i++) {
            header.push_back("feature_" + to_string(i));
            table[header[i]] = vector<string>{lineVec[i]};
        }
        numCols = lineVec.size();
        lineVec.clear();
    }

    uint32_t rowCounter = 0;
    while (true) {
        getline(fin, line);
        istringstream ss(line);
        while (getline(ss, item, delimiter)) {
            if (item[0] == '\"' && item[item.size() - 1] != '\"') {
                string next_item;
                getline(ss, next_item, delimiter);
                item += next_item;
            }
            item.erase(remove(item.begin(), item.end(), '\"'), item.end());
            lineVec.push_back(item);
        }
        if (fin.eof()) {
            break;
        } else {
            while (lineVec.size() < numCols) {
                lineVec.push_back("");
            }
            for (size_t i = 0; i < numCols; i++) {
                table[header[i]].push_back(lineVec[i]);
            }
            lineVec.clear();
            rowCounter++;
            if (rowCounter == maxNumRows) {
                break;
            }
        }
    }
    fin.close();

    return table;
}

// void writeTable(const vector<vector<uint64_t>> &table, const string
// &fileName,
//                 const vector<string> &header, const char delimiter) {
//     ofstream outfile;
//     outfile.open(fileName);
//     if (!outfile.is_open()) {
//         cerr << "Fail to open file" << endl;
//         exit(EXIT_FAILURE);
//     }

//     uint64_t num_rows = table.size();
//     uint64_t feature_num = table[0].size();

//     if (header.size() == feature_num) {
//         for (size_t j = 0; j < feature_num - 1; j++) {
//             outfile << header[j] << delimiter;
//         }
//         outfile << header[feature_num - 1] << endl;
//     }

//     for (size_t i = 0; i < num_rows; i++) {
//         for (size_t j = 0; j < feature_num - 1; j++) {
//             outfile << table[i][j] << delimiter;
//         }
//         outfile << table[i][feature_num - 1] << endl;
//     }
//     outfile.close();
// }

vector<string> concate1(const vector<string> &features,
                        map<string, vector<string>> &rawTable) {
    vector<string> result = rawTable[features[0]];
    for (size_t i = 1; i < features.size(); i++) {
        for (size_t k = 0; k < result.size(); k++) {
            if (result[k] != "" && rawTable[features[i]][k] != "") {
                result[k] += rawTable[features[i]][k];
            } else if (rawTable[features[i]][k] == "") {
                result[k] = "";
            }
        }
    }
    return result;
}

vector<string> concate2(const vector<string> &features,
                        map<string, vector<string>> &rawTable) {
    vector<string> result = rawTable[features[0]];
    for (size_t i = 1; i < features.size(); i++) {
        for (size_t k = 0; k < result.size(); k++) {
            result[k] += rawTable[features[i]][k];
        }
    }
    return result;
}

vector<byte> hashWithDummy(const vector<string> &column,
                           const uint32_t hash_size_in_byte,
                           const block dummyBlock) {
    vector<byte> hashes(column.size() * hash_size_in_byte);
    for (size_t i = 0; i < column.size(); i++) {
        if (column[i].size() == 0)  // if NaN
        {
            memcpy(hashes.data() + i * hash_size_in_byte, &dummyBlock,
                   hash_size_in_byte);
        } else {
            auto hash =
                XXH3_128bits_withSeed(column[i].data(), column[i].size(), 0);
            block hash_block = toBlock(hash.high64, hash.low64);
            memcpy(hashes.data() + i * hash_size_in_byte, &hash_block,
                   hash_size_in_byte);
        }
    }

    return hashes;
}

unordered_map<uint32_t, uint32_t> findLink(const vector<string> &id1,
                                           const vector<string> &id2) {
    unordered_map<string, int> idsToIdx;
    unordered_set<string> idsToSet;
    for (size_t i = 0; i < id2.size(); i++) {
        idsToIdx.insert(make_pair(id2[i], i));
        idsToSet.insert(id2[i]);
    }

    unordered_map<uint32_t, uint32_t> link;
    for (size_t i = 0; i < id1.size(); i++) {
        if (idsToSet.find(id1[i]) != idsToSet.end()) {
            link[i] = idsToIdx[id1[i]];
        }
    }

    return link;
}

vector<string> simulationPairWiseWithThreshold1(
    const vector<vector<string>> &myTable,
    const vector<vector<string>> &theirTable, const vector<string> &theirIDs) {
    size_t myNumRows = myTable.size();
    size_t theirNumRows = theirTable.size();
    size_t numFeatures = myTable[0].size();

    vector<unordered_map<string, size_t>> mapFromTheirTableToIdx(numFeatures);
    for (size_t i = 0; i < numFeatures; i++) {
        for (size_t j = 0; j < theirNumRows; j++) {
            if (theirTable[j][i] != "") {
                mapFromTheirTableToIdx[i].insert(
                    make_pair(theirTable[j][i], j));
            }
        }
    }

    vector<bool> isAssigned(myTable.size(), false);
    vector<string> myIDs(myTable.size(), "");
    for (int i = numFeatures - 1; i >= 0; i--) {
        unordered_map<string, bool> isFirst;
        for (size_t j = 0; j < myNumRows; j++) {
            isFirst[myTable[j][i]] = true;
        }
        for (size_t j = 0; j < myNumRows; j++) {
            if (!isAssigned[j]) {
                if (mapFromTheirTableToIdx[i].find(myTable[j][i]) !=
                    mapFromTheirTableToIdx[i].end()) {
                    size_t k = mapFromTheirTableToIdx[i][myTable[j][i]];
                    myIDs[j] = theirIDs[k];
                    isAssigned[j] = true;
                }
            }
        }
    }
    for (size_t i = 0; i < myNumRows; i++) {
        if (!isAssigned[i]) {
            myIDs[i] = randomString(32);
        }
    }

    return myIDs;
}

vector<string> simulationAlg1WithThreshold1(
    const vector<vector<string>> &myTable,
    const vector<vector<string>> &theirTable, const vector<string> &theirIDs) {
    size_t myNumRows = myTable.size();
    size_t theirNumRows = theirTable.size();
    size_t numFeatures = myTable[0].size();

    vector<unordered_map<string, string>> theirTableAsMap(numFeatures);
    for (size_t i = 0; i < numFeatures; i++) {
        for (size_t j = 0; j < theirNumRows; j++) {
            if (theirTable[j][i] != "") {
                theirTableAsMap[i][theirTable[j][i]] = theirIDs[j];
            }
        }
    }

    vector<bool> isAssigned(myTable.size(), false);
    vector<string> myIDs(myTable.size(), "");
    for (size_t i = 0; i < myNumRows; i++) {
        vector<string> candidateIDs;
        for (int j = numFeatures - 1; j >= 0; j--) {
            if (theirTableAsMap[j].find(myTable[i][j]) !=
                theirTableAsMap[j].end()) {
                myIDs[i] = theirTableAsMap[j][myTable[i][j]];
                isAssigned[i] = true;
                break;
            }
        }
    }
    for (size_t i = 0; i < myNumRows; i++) {
        if (!isAssigned[i]) {
            myIDs[i] = randomString(32);
        }
    }

    return myIDs;
}

void checkAccuracy(unordered_map<uint32_t, uint32_t> &link,
                   const vector<string> &derived_IDs,
                   const vector<string> &their_IDs) {
    uint32_t tp = 0, fn = 0, fp = 0, tn = 0;
    unordered_set<string> their_IDs_set(their_IDs.begin(), their_IDs.end());

    for (size_t i = 0; i < derived_IDs.size(); i++) {
        if (link.find(i) != link.end()) {
            if (derived_IDs[i] == their_IDs[link[i]])
                tp++;
            else
                fn++;
        } else {
            if (their_IDs_set.find(derived_IDs[i]) != their_IDs_set.end())
                fp++;
            else
                tn++;
        }
    }

    cout << "FN / TP / FP / TN : ";
    cout << fn << " / " << tp << " / " << fp << " / " << tn << endl;

    double acc = double(tp + tn) / double(tp + fp + fn + tn);
    double ppv = double(tp) / double(tp + fp);
    double recall = double(tp) / double(tp + fn);
    double f1 = double(2 * recall * ppv) / double(recall + ppv);

    cout << "# Acc : " << acc << endl;
    cout << "# Precision : " << ppv << endl;
    cout << "# Recall : " << recall << endl;
    cout << "# F1 : " << f1 << endl;
}

}  // namespace fuzzypc::common