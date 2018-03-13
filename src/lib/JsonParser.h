/*******************************************************************************
*   (c) 2016 Ledger
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

#ifndef CI_TEST_JSONPARSER_H
#define CI_TEST_JSONPARSER_H

#include "jsmn.h"
#include <stdbool.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned char byte;

#define MAX_NUMBER_OF_TOKENS    128
#define MAX_JSON_DEPTH          6
#define MAX_INPUT_OUTPUT_COUNT  2
#define MAX_COIN_COUNT          3

// Part of the SendMsg struct
typedef struct
{
    byte Denum;
    byte Amount;
} Coin;

// Part of the SendMsg struct
typedef struct
{
    byte Address;
    Coin Coins[MAX_COIN_COUNT];
    byte Sequence;
    byte NumberOfCoins;
} Input;

// Part of the SendMsg struct
typedef struct
{
    byte Address;
    Coin Coins[MAX_COIN_COUNT];
    byte NumberOfCoins;
} Output;

// Main structure that carries all the parsed data including:
//  - found tokens
//  - parent-child relationships between tokens
//  - SendMsg structure representation with links to tokens
typedef struct
{
    // Tokens
    bool        CorrectFormat;
    byte        NumberOfTokens;
    jsmntok_t   Tokens[MAX_NUMBER_OF_TOKENS];

    // SendMsg
    byte        NumberOfInputs;
    byte        NumberOfOutputs;
    Input       Inputs[MAX_INPUT_OUTPUT_COUNT];
    Output      Outputs[MAX_INPUT_OUTPUT_COUNT];
} parsed_json_t;

// The main function that parses jsonString and creates a parsed message
// which contains all the json tokens plus skeleton of SendMsg with links to those tokens
void ParseJson(parsed_json_t* parsedMessage, const char* jsonString);

// Helper function which is used to find parent-child relationship in the token hierarchy
// This function is called recursively from within ParseJson function
void ProcessToken(parsed_json_t* parsedMessage, int currentDepth, int tokenIndex);

#ifdef __cplusplus
}
#endif
#endif //CI_TEST_JSONPARSER_H
