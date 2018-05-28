/*******************************************************************************
*   (c) 2018 ZondaX GmbH
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
#include <iostream>


extern "C"
{
#include <lib/json_parser.h>
}

int main()
{
    parsed_json_t data;
    json_parse(&data, "Topic: \"Hello json world\"");

    std::cout << "Number of found tokens: " << data.NumberOfTokens << std::endl;

    return 0;
}