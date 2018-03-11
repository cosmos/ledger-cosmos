// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.
#include <jsmn.h>
#include "JsonParser.h"

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
// 2. The same block of code is duplicated for inputs and outputs - it needs some refactoring
// 3. Parsing logic supports varying number of inputs, outputs and coins. It also allowed to inputs and outputs be
//    in the stream in the arbitrary positions. It however expects Input and Coin to be in a specific format.
//    Additional flexibility can be achieved if necessary but at the cost of doing additional string comparisons.

bool IsChild(int potentialChildIndex, int parentIndex, jsmntok_t* tokens)
{
    return (tokens[potentialChildIndex].start > tokens[parentIndex].start)
        && (tokens[potentialChildIndex].end < tokens[parentIndex].end);
}

void ParseMessage(
        parsed_json_t* parsedMessage,
        const char* jsonString)
{
    const char inputsTokenName[] = "inputs";
    int inputsTokenSize = sizeof(inputsTokenName)-1; // minus null terminating char

    const char outputsTokenName[] = "outputs";
    int outputsTokenSize = sizeof(outputsTokenName)-1; // minus null terminating char

    bool processedInputs = false;
    bool processedOutputs = false;

    int i = 0;
    while (i < parsedMessage->NumberOfTokens) {
        if (parsedMessage->Tokens[i].type == JSMN_UNDEFINED){
            break;
        }
        // Parse inputs array
        if (!processedInputs && Match(jsonString, parsedMessage->Tokens[i], inputsTokenName, inputsTokenSize)) {
            int inputsArrayIndex = i + 1;
            parsedMessage->NumberOfInputs = 0;

            // Parse input
            int inputIndex = inputsArrayIndex + 1;
            while (IsChild(inputIndex, inputsArrayIndex, parsedMessage->Tokens)) {

                // Parse single input
                parsedMessage->Inputs[parsedMessage->NumberOfInputs].Address = inputIndex + 2;

                // Parse Coins array
                int coinsArrayIndex = inputIndex + 4;
                parsedMessage->Inputs[parsedMessage->NumberOfInputs].NumberOfCoins = 0;

                // Parse Coin
                int coinIndex = coinsArrayIndex + 1;
                while (IsChild(coinIndex, coinsArrayIndex, parsedMessage->Tokens)) {

                    parsedMessage->Inputs[parsedMessage->NumberOfInputs].Coins[parsedMessage->Inputs[parsedMessage->NumberOfInputs].NumberOfCoins].Denum =
                            coinIndex + 2;

                    parsedMessage->Inputs[parsedMessage->NumberOfInputs].Coins[parsedMessage->Inputs[parsedMessage->NumberOfInputs].NumberOfCoins++].Amount =
                            coinIndex + 4;

                    coinIndex += 5;
                }

                parsedMessage->Inputs[parsedMessage->NumberOfInputs].Sequence = coinIndex + 1;
                parsedMessage->NumberOfInputs++;

                inputIndex = coinIndex + 2;
            }
            i = inputIndex - 1;
            processedInputs = true;
        }

        // Parse outputs array
        if (!processedOutputs && Match(jsonString, parsedMessage->Tokens[i], outputsTokenName, outputsTokenSize)) {
            int outputsArrayIndex = i + 1;
            parsedMessage->NumberOfOutputs = 0;

            // Parse output
            int outputIndex = outputsArrayIndex + 1;
            while (IsChild(outputIndex, outputsArrayIndex, parsedMessage->Tokens)) {

                // Parse single input
                parsedMessage->Outputs[parsedMessage->NumberOfOutputs].Address = outputIndex + 2;

                // Parse Coins array
                int coinsArrayIndex = outputIndex + 4;
                parsedMessage->Outputs[parsedMessage->NumberOfOutputs].NumberOfCoins = 0;

                // Parse Coin
                int coinIndex = coinsArrayIndex + 1;
                while (IsChild(coinIndex, coinsArrayIndex, parsedMessage->Tokens)) {

                    parsedMessage->Outputs[parsedMessage->NumberOfOutputs].Coins[parsedMessage->Outputs[parsedMessage->NumberOfOutputs].NumberOfCoins].Denum =
                            coinIndex + 2;

                    parsedMessage->Outputs[parsedMessage->NumberOfOutputs].Coins[parsedMessage->Outputs[parsedMessage->NumberOfOutputs].NumberOfCoins++].Amount =
                            coinIndex + 4;

                    coinIndex += 5;
                }

                parsedMessage->NumberOfOutputs++;
                outputIndex = coinIndex;
            }
            i = outputIndex - 1;
            processedOutputs = true;
        }

        i++;
    }
}

    void ParseJson(parsed_json_t *parsedJson, const char *jsonString) {
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

        // Build SendMsg representation with links to tokens
        ParseMessage(parsedJson, jsonString);
    }