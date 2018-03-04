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

typedef struct
{
    bool        CorrectFormat;
    int         NumberOfTokens;
    jsmntok_t   Tokens[MAX_NUMBER_OF_TOKENS];
} JsonParserData;

void ParseJson(JsonParserData* jsonParserData, const char* jsonString);

#ifdef __cplusplus
}
#endif
#endif //CI_TEST_JSONPARSER_H
