// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
#include <jsmn.h>
//#include <c++/v1/array>
#include "JsonParser.h"

void ProcessToken(ParsedMessage* parsedMessage,
                  int currentDepth,
                  int tokenIndex)
{
    if (tokenIndex >= parsedMessage->NumberOfTokens
        ||
        currentDepth < 0)
    {
        return;
    }
    if (parsedMessage->Tokens[tokenIndex].start > parsedMessage->Tokens[parsedMessage->TokensInfo.CurrentTraversalPath[currentDepth]].start
        &&
        parsedMessage->Tokens[tokenIndex].end < parsedMessage->Tokens[parsedMessage->TokensInfo.CurrentTraversalPath[currentDepth]].end)
    {
        parsedMessage->TokensInfo.Parents[tokenIndex] = parsedMessage->TokensInfo.CurrentTraversalPath[currentDepth];

        currentDepth++;
        parsedMessage->TokensInfo.CurrentTraversalPath[currentDepth] = tokenIndex;

        ProcessToken(
                parsedMessage,
                currentDepth,
                tokenIndex + 1);
    }
    else
    {
        currentDepth--;

        ProcessToken(
                parsedMessage,
                currentDepth,
                tokenIndex);
    }
}

bool Match(
        const char* jsonString,
        jsmntok_t token,
        const char* reference,
        int size)
{
    bool sizeOkay =(token.end - token.start) == size;
    const char* str = jsonString + token.start;
    return (sizeOkay && strncmp(str, reference, size) == 0);
}

// Known issues:
// 1. This function should scan the whole token list only once to get all information
//    At the moment it's doing slightly more work because in few cases it's moving back as it's scanning
// 2. The same block of coed is duplicated for inputs and outputs - refactor
// 3. Parsing logic supports varying number of inputs, outputs and coins. It also allowed to inputs and outputs be
//    in the stream in the arbitrary positions. It however expects Input and Coin to be in a specific format.
//    Additional flexibility can be achieved if necessary but at the cost of doing additional string comparisons.

void ParseMessage(
        ParsedMessage* parsedMessage,
        const char* jsonString)
{
    const char inputsTokenName[] = "inputs";
    int inputsTokenSize = sizeof(inputsTokenName)-1; // minus null terminating char

    const char outputsTokenName[] = "outputs";
    int outputsTokenSize = sizeof(outputsTokenName)-1; // minus null terminating char

    bool processedInputs = false;
    bool processedOutputs = false;

    for (int i = 0; i < parsedMessage->NumberOfTokens; i++) {
        // Find inputs token
        if (!processedInputs && Match(jsonString, parsedMessage->Tokens[i], inputsTokenName, inputsTokenSize)) {
            int inputsIndex = i + 1;
            parsedMessage->NumberOfInputs = 0;
            // Search for all the inputs
            for (int j = inputsIndex + 1; j < parsedMessage->NumberOfTokens; j++) {
                // Bail out if current token is already outside inputs range
                if (parsedMessage->Tokens[j].end >= parsedMessage->Tokens[inputsIndex].end) break;
                // Find the start of a input
                if (parsedMessage->TokensInfo.Parents[j] == inputsIndex) {
                    parsedMessage->Inputs[parsedMessage->NumberOfInputs].Address = j + 2;
                    int coinsIndex = j + 4;
                    parsedMessage->Inputs[parsedMessage->NumberOfInputs].NumberOfCoins = 0;
                    //Search for all the coins
                    for (int m = coinsIndex + 1; m < parsedMessage->NumberOfTokens; m++) {
                        //Bail out if the current token is already outside coins range
                        if (parsedMessage->Tokens[m].end >= parsedMessage->Tokens[j].end) break;
                        // Find the start of a coin
                        if (parsedMessage->TokensInfo.Parents[m] == coinsIndex) {
                            parsedMessage->Inputs[parsedMessage->NumberOfInputs].Coins[parsedMessage->Inputs[parsedMessage->NumberOfInputs].NumberOfCoins].Denum = m + 2;
                            parsedMessage->Inputs[parsedMessage->NumberOfInputs].Coins[parsedMessage->Inputs[parsedMessage->NumberOfInputs].NumberOfCoins++].Amount = m + 4;
                        }
                    }
                    parsedMessage->NumberOfInputs++;
                }
            }
            processedInputs = true;
        }
        if (!processedOutputs && Match(jsonString, parsedMessage->Tokens[i], outputsTokenName, outputsTokenSize)) {
            int outputsIndex = i + 1;
            parsedMessage->NumberOfOutputs = 0;
            // Search for all the inputs
            for (int j = outputsIndex + 1; j < parsedMessage->NumberOfTokens; j++) {
                // Bail out if current token is already outside inputs range
                if (parsedMessage->Tokens[j].end >= parsedMessage->Tokens[outputsIndex].end) break;
                // Find the start of a input
                if (parsedMessage->TokensInfo.Parents[j] == outputsIndex) {
                    parsedMessage->Outputs[outputsIndex].Address = j + 2;
                    int coinsIndex = j + 4;
                    parsedMessage->Outputs[parsedMessage->NumberOfOutputs].NumberOfCoins = 0;
                    //Search for all the coins
                    for (int m = coinsIndex + 1; m < parsedMessage->NumberOfTokens; m++) {
                        //Bail out if the current token is already outside coins range
                        if (parsedMessage->Tokens[m].end >= parsedMessage->Tokens[j].end) break;
                        // Find the start of a coin
                        if (parsedMessage->TokensInfo.Parents[m] == coinsIndex) {
                            parsedMessage->Outputs[parsedMessage->NumberOfOutputs].Coins[parsedMessage->Outputs[parsedMessage->NumberOfOutputs].NumberOfCoins].Denum = m + 2;
                            parsedMessage->Outputs[parsedMessage->NumberOfOutputs].Coins[parsedMessage->Outputs[parsedMessage->NumberOfOutputs].NumberOfCoins++].Amount = m + 4;
                        }
                    }
                    parsedMessage->NumberOfOutputs++;
                }
            }
            processedOutputs = true;
        }
    }
}

    void ParseJson(ParsedMessage *parsedJson, const char *jsonString) {
        jsmn_parser parser;
        jsmn_init(&parser);

        parsedJson->NumberOfTokens = jsmn_parse(
                &parser,
                jsonString,
                strlen(jsonString),
                parsedJson->Tokens,
                MAX_NUMBER_OF_TOKENS);

        parsedJson->CorrectFormat = false;
        if (parsedJson->NumberOfTokens >= 1
            &&
            parsedJson->Tokens[0].type != JSMN_OBJECT) {
            parsedJson->CorrectFormat = true;
        }

        // Recursively build token relationships
        ProcessToken(parsedJson, 0, 1);

        // Build SendMsg representation with links to tokens
        ParseMessage(parsedJson, jsonString);
    }