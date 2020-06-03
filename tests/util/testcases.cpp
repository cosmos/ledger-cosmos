/*******************************************************************************
*   (c) 2019 Zondax GmbH
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
********************************************************************************/
#include "testcases.h"
#include <fmt/core.h>
#include <gtest/gtest.h>
#include <algorithm>

std::vector<testcase_t> GetJsonTestCases(const std::string &filename) {
    auto answer = std::vector<testcase_t>();

    Json::CharReaderBuilder builder;
    std::shared_ptr<Json::Value> obj(new Json::Value());

    std::ifstream inFile(filename);
    EXPECT_TRUE(inFile.is_open())
                        << "\n"
                        << "******************\n"
                        << "Check that your working directory points to the tests directory\n"
                        << "In CLion use $PROJECT_DIR$\\tests\n"
                        << "******************\n";
    if (!inFile.is_open())
        return answer;

    // Retrieve all test cases
    JSONCPP_STRING errs;
    Json::parseFromStream(builder, inFile, obj.get(), &errs);
    std::cout << "Number of testcases: " << obj->size() << std::endl;
    answer.reserve(obj->size());

    for (int i = 0; i < obj->size(); i++) {
        auto v = (*obj)[i];

        Json::StreamWriterBuilder wbuilder;
        wbuilder["commentStyle"] = "None";
        wbuilder["indentation"] = "";
        std::string txStr = Json::writeString(wbuilder, v["tx"]);

        auto expected = std::vector<std::string>();
        for (const auto j : v["expected"]) {
            expected.push_back(j.asString());
        }

        bool expert = false;
        expert = v["expert"].asBool();

        answer.push_back(
                testcase_t{
                        v["name"].asString(),
                        txStr,
                        v["parsingErr"].asString(),
                        v["validationErr"].asString(),
                        expected,
                        expert
                });
    }

    return answer;
}
