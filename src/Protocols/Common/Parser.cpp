/* 본 프로그램에 대한 저작권을 포함한 지적재산권은 삼성SDS(주)에 있으며,
삼성SDS(주)가 명시적으로 허용하지 않은 사용, 복사, 변경, 제3자에의 공개,
배포는 엄격히 금지되며, 삼성SDS(주)의 지적재산권 침해에 해당됩니다.
Copyright (c) 2023 Samsung SDS Co., Ltd. All Rights Reserved. Confidential.
All information including the intellectual and technical concepts contained
herein is, and remains the property of Samsung SDS Co. Ltd. Unauthorized use,
dissemination, or reproduction of this material is strictly forbidden unless
prior written permission is obtained from Samsung SDS Co. Ltd.
*/

#include "Parser.h"

#include <fstream>
#include <set>
#include <sstream>

#include "RapidCsv.h"

using namespace std;

namespace fuzzypc::common {
size_t readTable(const string &filePath, map<string, vector<string>> &table,
                 const int columnNameIndex, const int rowNameIndex,
                 const char delimiter) {
    rapidcsv::LabelParams params;
    params.mColumnNameIdx = columnNameIndex;
    params.mRowNameIdx = rowNameIndex;
    rapidcsv::Document document(filePath, params,
                                rapidcsv::SeparatorParams(delimiter));
    auto header = document.GetColumnNames();
    for (auto key : header) {
        table[key] = document.GetColumn<string>(key);
    }
    return document.GetRowCount();
}

void writeTable(const vector<vector<uint64_t>> &table, const string &filePath,
                const vector<string> &header, const char delimiter) {
    rapidcsv::Document document("", rapidcsv::LabelParams(0, -1),
                                rapidcsv::SeparatorParams(delimiter));
    size_t numRows = table.size();
    size_t numCols = header.size();
    for (size_t i = 0; i < numCols; i++) {
        document.SetColumnName(i, header[i]);
    }
    for (size_t i = 0; i < numRows; i++) {
        document.SetRow<uint64_t>(i, table[i]);
    }
    document.Save(filePath);
}

void writeTable(const vector<vector<double>> &table, const string &filePath,
                const vector<string> &header, const char delimiter) {
    rapidcsv::Document document("", rapidcsv::LabelParams(0, -1),
                                rapidcsv::SeparatorParams(delimiter));
    size_t numCols = header.size();
    for (size_t i = 0; i < numCols; i++) {
        document.SetColumnName(i, header[i]);
        document.SetColumn<double>(header[i], table[i]);
    }
    document.Save(filePath);
}
}  // namespace fuzzypc::common