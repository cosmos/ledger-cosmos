// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
#include <jsmn.h>
#include "JsonParser.h"

void ParseJson(JsonParserData* jsonParserData, const char* jsonString)
{
    jsmn_parser parser;
    jsmn_init(&parser);

    jsonParserData->NumberOfTokens = jsmn_parse(&parser, jsonString, strlen(jsonString), jsonParserData->Tokens, MAX_NUMBER_OF_TOKENS);

    jsonParserData->CorrectFormat = false;
    if (jsonParserData->NumberOfTokens >= 1 && jsonParserData->Tokens[0].type != JSMN_OBJECT)
    {
        jsonParserData->CorrectFormat = true;
    }

    // TODO Add logic to get number of input and outputs and find relative indices for all the nodes
}