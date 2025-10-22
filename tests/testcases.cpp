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
#include <algorithm>
#include <fmt/core.h>
#include <gtest/gtest.h>

std::vector<testcase_t> GetJsonTestCases(const std::string &jsonFile) {
  auto answer = std::vector<testcase_t>();

  const std::string fullPathJsonFile = std::string(TESTVECTORS_DIR) + jsonFile;

  std::ifstream inFile(fullPathJsonFile);
  if (!inFile.is_open()) {
    return answer;
  }

  // Retrieve all test cases
  nlohmann::json obj;
  inFile >> obj;
  std::cout << "Number of testcases: " << obj.size() << std::endl;
  answer.reserve(obj.size());

  for (size_t i = 0; i < obj.size(); i++) {
    auto v = obj[i];

    // Match jsoncpp format: no indentation, no spaces after colons
    // ensure_ascii=true to escape non-ASCII characters (like emojis) to \uXXXX
    // format
    const std::string txStr =
        v["tx"].dump(-1, ' ', true, nlohmann::json::error_handler_t::replace);

    auto expected = std::vector<std::string>();
    for (const auto &j : v["expected"]) {
      expected.push_back(j.get<std::string>());
    }

    bool expert = false;
    if (v.contains("expert")) {
      expert = v["expert"].get<bool>();
    }

    answer.push_back(testcase_t{0, v["name"].get<std::string>(), txStr,
                                v["parsingErr"].get<std::string>(),
                                v["validationErr"].get<std::string>(), expected,
                                expected, expert});
  }

  return answer;
}

std::vector<testcase_t> GetJsonTextualTestCases(const std::string &jsonFile) {
  auto answer = std::vector<testcase_t>();

  const std::string fullPathJsonFile = std::string(TESTVECTORS_DIR) + jsonFile;

  std::ifstream inFile(fullPathJsonFile);
  if (!inFile.is_open()) {
    return answer;
  }

  // Retrieve all test cases
  nlohmann::json obj;
  inFile >> obj;
  std::cout << "Number of testcases: " << obj.size() << std::endl;

  for (size_t i = 0; i < obj.size(); i++) {

    auto outputs = std::vector<std::string>();
    for (const auto &s : obj[i]["output"]) {
      outputs.push_back(s.get<std::string>());
    }

    auto outputs_expert = std::vector<std::string>();
    for (const auto &s : obj[i]["output_expert"]) {
      outputs_expert.push_back(s.get<std::string>());
    }

    answer.push_back(testcase_t{obj[i]["index"].get<uint64_t>(),
                                obj[i]["name"].get<std::string>(),
                                obj[i]["blob"].get<std::string>(), "", "",
                                outputs, outputs_expert, false});
  }
  return answer;
}
