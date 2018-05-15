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

// Coin part of the SendMsg struct
typedef struct
{
    byte Denum;     //< index that points to Denum token in parsed json
    byte Amount;    //< index that points to Amount token in parsed json
} Coin;

// Input part of the SendMsg struct
typedef struct
{
    byte Address;               //< index that points to Denum token in parsed json
    Coin Coins[MAX_COIN_COUNT];
    //byte Sequence;              //< index that points to Sequence token in parsed json
    byte NumberOfCoins;
} Input;

// Output part of the SendMsg struct
typedef struct
{
    byte Address;               //< index that points to Address in parsed json
    Coin Coins[MAX_COIN_COUNT];
    byte NumberOfCoins;
} Output;

// Context that keeps all the parsed data together. That includes:
//  - parsed json tokens
//  - re-created SendMsg struct with indices pointing to tokens in parsed json
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

// Get key/value pairs (name/value) from the transaction message based on the
// index of the key/value pair that will be displayed
int TransactionMsgGetInfo(
        char *name,
        char *value,
        int index,
        const parsed_json_t* parsed_transaction,
        unsigned int* view_scrolling_total_size,
        unsigned int view_scrolling_step,
        unsigned int max_chars_per_line,
        const char* message,
        void(*copy)(char* dst, const char* source, unsigned int size));


#ifdef __cplusplus
}
#endif
#endif //CI_TEST_JSONPARSER_H
