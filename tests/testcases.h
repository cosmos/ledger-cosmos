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
#pragma once
#include <fstream>
#include <nlohmann/json.hpp>

typedef struct {
  uint64_t index; // textual mode
  std::string name;
  std::string tx;
  std::string parsingErr;
  std::string validationErr;
  std::vector<std::string> expected;
  std::vector<std::string> expected_expert; // textual mode
  bool expert;
} testcase_t;

std::vector<testcase_t> GetJsonTestCases(const std::string &filename);
std::vector<testcase_t> GetJsonTextualTestCases(const std::string &jsonFile);
