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

#define MAX_NUMBER_OF_TOKENS 128

#define DF_NAME_INDEX                   1 //(string)
#define V_NAME_INDEX                    3 //(string)
#define V_VALUE_INDEX                   4 //(object)
#define INPUTS_NAME_INDEX               5 //(string)
#define INPUTS_VALUE_INDEX              6 //(array)

    // Repeated for every input in the inputs array
    #define INPUT_VALUE_INDEX               7 //(object)
    #define INPUT_ADDRESS_NAME_INDEX        8 //(string)
    #define INPUT_ADDRESS_VALUE_INDEX       9 //(string)
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


typedef struct
{
    bool        CorrectFormat;
    int         NumberOfTokens;
    jsmntok_t   Tokens[MAX_NUMBER_OF_TOKENS];
    int         NumberOfInuts;
    int         NumberOfOutputs;
} JsonParserData;

void ParseJson(JsonParserData* jsonParserData, const char* jsonString);

static int GetNumberOfInputs(JsonParserData* jsonData)
{
    return 0;
}

static jsmntok_t GetInputAddressToken(JsonParserData* jsonData, int inputNumber)
{
    return jsonData->Tokens[0];
}

static int GetInputNumberOfCoins(JsonParserData* jsonData, int inputNumber)
{
    return 0;
}

static jsmntok_t GetInputCoinDenomToken(JsonParserData* jsonData, int inputNumber, int coinNumber)
{
    return jsonData->Tokens[0];
}

static jsmntok_t GetInputCoinAmountToken(JsonParserData* jsonData, int inputNumber, int coinNumber)
{
    return jsonData->Tokens[0];
}

static int GetNumberOfOutputs(JsonParserData* jsonData)
{
    return 0;
}

static jsmntok_t GetOutputAddressToken(JsonParserData* jsonData, int outputNumber)
{
    return jsonData->Tokens[0];
}

static int GetOutputNumberOfCoints(JsonParserData* jsonData, int outputNumber)
{
    return 0;
}

static jsmntok_t GetOutputCoinDenomToken(JsonParserData* jsonData, int outputNumber, int coinNumber)
{
    return jsonData->Tokens[0];
}

static jsmntok_t GetOutputCoinAmountToken(JsonParserData* jsonData, int outputNumber, int coinNumber)
{
    return jsonData->Tokens[0];
}

#ifdef __cplusplus
}
#endif
#endif //CI_TEST_JSONPARSER_H
