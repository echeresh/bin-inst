#pragma once
#include <algorithm>
#include <cassert>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace streamutils
{
    class Table
    {
        std::vector<std::string> columns;
        std::vector<std::vector<std::string>> rows;

    public:
        void addColumn(const std::string& columnName)
        {
            columns.push_back(columnName);
        }

        void addRow(const std::vector<std::string>& values)
        {
            assert(values.size() == columns.size());
            rows.push_back(values);
        }

        std::string str() const
        {
            std::vector<int> columnSizes(columns.size());
            for (size_t i = 0; i < columns.size(); i++)
            {
                columnSizes[i] = (int)columns[i].length();
            }

            for (size_t i = 0; i < rows.size(); i++)
            {
                for (size_t j = 0; j < columns.size(); j++)
                {
                    columnSizes[j] = std::max(columnSizes[j], (int)rows[i][j].length());

                }
            }

            for (size_t i = 0; i < columns.size(); i++)
            {
                columnSizes[i] += 2;
            }

            std::ostringstream oss;
            for (size_t i = 0; i < columns.size(); i++)
            {
                oss << std::setw(columnSizes[i]) << columns[i];
            }
            oss << std::endl;
            for (size_t i = 0; i < rows.size(); i++)
            {
                for (size_t j = 0; j < columns.size(); j++)
                {
                    oss << std::setw(columnSizes[j]) << rows[i][j];
                }
                oss << std::endl;
            }
            return oss.str();
        }
    };
} //namespace streamutils
