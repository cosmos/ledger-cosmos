// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
#ifndef CI_TEST_JSONPARSER_H
#define CI_TEST_JSONPARSER_H

#include "jsmn.h"
#include <stdbool.h>
#include <string.h>


#ifdef __cplusplus
extern "C" {
#endif

#define MAX_NUMBER_OF_TOKENS    128
#define MAX_JSON_DEPTH          10
#define MAX_INPUT_OUTPUT_COUNT  5
#define MAX_COIN_COUNT          5

// List of JSON element in SendMsg (begin)
// TODO: Remove it
#define DF_NAME_INDEX                   1 //(string)
#define V_NAME_INDEX                    3 //(string)
#define V_VALUE_INDEX                   4 //(object)
#define INPUTS_NAME_INDEX               5 //(string)
#define INPUTS_VALUE_INDEX              6 //(array)

// Repeated for every input in the inputs array
#define INPUT_VALUE_INDEX               7  //(object)
#define INPUT_ADDRESS_NAME_INDEX        8  //(string)
#define INPUT_ADDRESS_VALUE_INDEX       9  //(string)
#define INPUT_COINS_NAME_INDEX          10 //(string)
#define INPUT_COINS_VALUE_INDEX         11 //(array)

// Repeated for every coin in the coins array
#define INPUT_COIN_INDEX                12 //(object)
#define INPUT_COIN_DENOM_NAME_INDEX     13 //(object)
#define INPUT_COIN_DENOM_VALUE_INDEX    14 //(object)
#define INPUT_COIN_AMOUNT_NAME_INDEX    15 //(string)
#define INPUT_COIN_AMOUNT_VALUE_INDEX   16 //(primitive)

#define INPUT_SEQUENCE_NAME_INDEX       17 //(string)
#define INPUT_SEQUENCE_VALUE_INDEX      18 //(primitive)

#define OUTPUTS_NAME_INDEX               19 //(string)
#define OUTPUTS_VALUE_INDEX              20 //(array)

// Repeated for every input in the inputs array
#define OUTPUT_VALUE_INDEX               21 //(object)
#define OUTPUT_ADDRESS_NAME_INDEX        22 //(string)
#define OUTPUT_ADDRESS_VALUE_INDEX       23 //(string)
#define OUTPUT_COINS_NAME_INDEX          24 //(string)
#define OUTPUT_COINS_VALUE_INDEX         25 //(array)

// Repeated for every coin in the coins array
#define OUTPUT_COIN_INDEX                26 //(object)
#define OUTPUT_COIN_DENOM_NAME_INDEX     27 //(object)
#define OUTPUT_COIN_DENOM_VALUE_INDEX    28 //(object)
#define OUTPUT_COIN_AMOUNT_NAME_INDEX    29 //(string)
#define OUTPUT_COIN_AMOUNT_VALUE_INDEX   30 //(primitive)
// List of JSON element in SendMsg (end)

// Part of the SendMsg struct
typedef struct
{
    int Denum;
    int Amount;
} Coin;

// Part of the SendMsg struct
typedef struct
{
    int Address;
    Coin Coins[MAX_COIN_COUNT];
    int Sequence;
    int NumberOfCoins;
} Input;

// Part of the SendMsg struct
typedef struct
{
    int Address;
    Coin Coins[MAX_COIN_COUNT];
    int NumberOfCoins;
} Output;

// Helper struct that holds information about parent-child relationship between tokens
typedef struct
{
    // Array that holds a list of token that represent
    // currently traversed token path in json hierarchy
    int CurrentTraversalPath[MAX_JSON_DEPTH];

    // Array that maps token indices to their corresponding parent indices
    // For example: parent[3] = 2, means that token with index 2 is a parent of token with index 2
    int Parents[MAX_NUMBER_OF_TOKENS];
} TokenInfo;

// Main structure that carries all the parsed data including:
//  - found tokens
//  - parent-child relationships between tokens
//  - SendMsg structure representation with links to tokens
typedef struct
{
    // Tokens
    bool        CorrectFormat;
    int         NumberOfTokens;
    jsmntok_t   Tokens[MAX_NUMBER_OF_TOKENS];
    TokenInfo   TokensInfo;

    // SendMsg
    int         NumberOfInputs;
    int         NumberOfOutputs;
    Input       Inputs[MAX_INPUT_OUTPUT_COUNT];
    Output      Outputs[MAX_INPUT_OUTPUT_COUNT];
} ParsedMessage;

// The main function that parses jsonString and creates a parsed message
// which contains all the json tokens plus skeleton of SendMsg with links to those tokens
void ParseJson(ParsedMessage* parsedMessage, const char* jsonString);

// Helper function which is used to find parent-child relationship in the token hierarchy
// This function is called recursively from within ParseJson function
void ProcessToken(ParsedMessage* parsedMessage, int currentDepth, int tokenIndex);

#ifdef __cplusplus
}
#endif
#endif //CI_TEST_JSONPARSER_H
