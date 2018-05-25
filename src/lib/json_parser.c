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

#include <jsmn.h>
#include "json_parser.h"

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
// 1. The same block of code is duplicated for inputs and outputs - this could be further refactored
// 2. Parsing logic supports a varying number of inputs, outputs and coins. Input and output arrays can be
//    found at random positions in the stream - offsets are not hardcoded. Internal layouts of individual
//    inputs and outputs are however hardcoded and must not change. 

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

                //parsedMessage->Inputs[parsedMessage->NumberOfInputs].Sequence = coinIndex + 1;
                parsedMessage->NumberOfInputs++;

                inputIndex = coinIndex;
            }
            i = inputIndex;
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

void ParseSignedMsg(parsed_json_t* parsedMessage, const char* signedMsg)
{
    jsmn_parser parser;
    jsmn_init(&parser);

    parsedMessage->NumberOfTokens = jsmn_parse(
            &parser,
            signedMsg,
            strlen(signedMsg),
            parsedMessage->Tokens,
            MAX_NUMBER_OF_TOKENS);

    parsedMessage->CorrectFormat = false;
    if (parsedMessage->NumberOfTokens >= 1
        &&
        parsedMessage->Tokens[0].type != JSMN_OBJECT) {
        parsedMessage->CorrectFormat = true;
    }

}

int TransactionMsgGetInfo(
        char *name,
        char *value,
        int index,
        const parsed_json_t* parsed_transaction,
        unsigned int* view_scrolling_total_size,
        unsigned int view_scrolling_step,
        unsigned int max_chars_per_line,
        const char* message,
        void(*copy)(void* dst, const void* source, unsigned int size))
{
    int currentIndex = 0;
    for (int i = 0; i < parsed_transaction->NumberOfInputs; i++) {
        if (index == currentIndex) {
            copy((char *) name, "Input address", sizeof("Input address"));

            unsigned int addressSize =
                    parsed_transaction->Tokens[parsed_transaction->Inputs[i].Address].end -
                    parsed_transaction->Tokens[parsed_transaction->Inputs[i].Address].start;

            *view_scrolling_total_size = addressSize;
            const char *addressPtr =
                    message +
                    parsed_transaction->Tokens[parsed_transaction->Inputs[i].Address].start;

            if (view_scrolling_step < addressSize) {
                copy(
                        (char *) value,
                        addressPtr + view_scrolling_step,
                        addressSize < max_chars_per_line ? addressSize : max_chars_per_line);
                value[addressSize] = '\0';
            }
            return currentIndex;
        }
        currentIndex++;
        for (int j = 0; j < parsed_transaction->Inputs[i].NumberOfCoins; j++) {
            if (index == currentIndex) {
                copy((char *) name, "Coin", sizeof("Coin"));

                int coinSize =
                        parsed_transaction->Tokens[parsed_transaction->Inputs[i].Coins[j].Denum].end -
                        parsed_transaction->Tokens[parsed_transaction->Inputs[i].Coins[j].Denum].start;

                const char *coinPtr =
                        message +
                        parsed_transaction->Tokens[parsed_transaction->Inputs[i].Coins[j].Denum].start;

                copy((char *) value, coinPtr, coinSize);
                value[coinSize] = '\0';
                return currentIndex;
            }
            currentIndex++;
            if (index == currentIndex) {
                copy((char *) name, "Amount", sizeof("Amount"));

                int coinAmountSize =
                        parsed_transaction->Tokens[parsed_transaction->Inputs[i].Coins[j].Amount].end -
                        parsed_transaction->Tokens[parsed_transaction->Inputs[i].Coins[j].Amount].start;

                const char *coinAmountPtr =
                        message +
                        parsed_transaction->Tokens[parsed_transaction->Inputs[i].Coins[j].Amount].start;

                copy((char *) value, coinAmountPtr, coinAmountSize);
                value[coinAmountSize] = '\0';
                return currentIndex;
            }
            currentIndex++;
        }
    }
    for (int i = 0; i < parsed_transaction->NumberOfOutputs; i++) {
        if (index == currentIndex) {
            copy((char *) name, "Output address", sizeof("Output address"));

            unsigned int addressSize =
                    parsed_transaction->Tokens[parsed_transaction->Outputs[i].Address].end -
                    parsed_transaction->Tokens[parsed_transaction->Outputs[i].Address].start;

            const char *addressPtr =
                    message +
                    parsed_transaction->Tokens[parsed_transaction->Outputs[i].Address].start;

            *view_scrolling_total_size = addressSize;

            if (view_scrolling_step < addressSize) {
                copy(
                        (char *) value,
                        addressPtr + view_scrolling_step,
                        addressSize < max_chars_per_line ? addressSize : max_chars_per_line);
                value[addressSize] = '\0';
            }
            return currentIndex;
        }
        currentIndex++;
        for (int j = 0; j < parsed_transaction->Outputs[i].NumberOfCoins; j++) {
            if (index == currentIndex) {
                copy((char *) name, "Coin", sizeof("Coin"));

                int coinSize =
                        parsed_transaction->Tokens[parsed_transaction->Outputs[i].Coins[j].Denum].end -
                        parsed_transaction->Tokens[parsed_transaction->Outputs[i].Coins[j].Denum].start;

                const char *coinPtr =
                        message +
                        parsed_transaction->Tokens[parsed_transaction->Outputs[i].Coins[j].Denum].start;

                copy((char *) value, coinPtr, coinSize);
                value[coinSize] = '\0';
                return currentIndex;
            }
            currentIndex++;
            if (index == currentIndex) {
                copy((char *) name, "Amount", sizeof("Amount"));

                int coinAmountSize =
                        parsed_transaction->Tokens[parsed_transaction->Outputs[i].Coins[j].Amount].end -
                        parsed_transaction->Tokens[parsed_transaction->Outputs[i].Coins[j].Amount].start;

                const char *coinAmountPtr =
                        message +
                        parsed_transaction->Tokens[parsed_transaction->Outputs[i].Coins[j].Amount].start;

                copy((char *) value, coinAmountPtr, coinAmountSize);
                value[coinAmountSize] = '\0';
                return currentIndex;
            }
            currentIndex++;
        }
    }
    return currentIndex;
}

int SignedMsgGetNumberOfElements(const parsed_json_t* parsed_transaction,
                                 const char* transaction)
{
    int token_index = object_get_value(0, "msg_bytes", parsed_transaction, transaction);
    return display_get_arbitrary_items(
            token_index,
            parsed_transaction,
            transaction) + 3;
}

int GetTokenIndexForKey(
        int keyIndex,
        const parsed_json_t* parsed_message)
{
    int currentIndex = 0;
    int foundKeyIndex = 0;
    int nextKeyMinPosition = -1;
    while (currentIndex < parsed_message->NumberOfTokens) {

        if (parsed_message->Tokens[currentIndex].type == JSMN_UNDEFINED) break;
        // Here we're assuming that json keys are always of JSMN_STRING type
        if (parsed_message->Tokens[currentIndex].type == JSMN_STRING) {
            if (parsed_message->Tokens[currentIndex].start > nextKeyMinPosition) {

                // Found the key
                if (keyIndex == foundKeyIndex) {
                    return currentIndex;
                }
                foundKeyIndex++;
                // Skip all the value tokens
                nextKeyMinPosition = parsed_message->Tokens[currentIndex + 1].end;
            }
        }
        currentIndex++;
    }
    return -1;
}

int SignedMsgGetInfo(
        char *name,
        char *value,
        int index,
        const parsed_json_t* parsed_message,
        unsigned int* view_scrolling_total_size,
        unsigned int view_scrolling_step,
        unsigned int max_chars_per_line,
        const char* message,
        void(*copy)(void* dst, const void* source, unsigned int size))
{
    int tokenKeyIndex = GetTokenIndexForKey(index, parsed_message);

    if (tokenKeyIndex != -1) {
        jsmntok_t key_token = parsed_message->Tokens[tokenKeyIndex];
        jsmntok_t value_token = parsed_message->Tokens[tokenKeyIndex+1];

        copy((char *) name,
             message + key_token.start,
             key_token.end - key_token.start);
        name[key_token.end - key_token.start] = '\0';

        *view_scrolling_total_size = value_token.end-value_token.start;

        const char* value_start_address = message + value_token.start;
        if (view_scrolling_step < *view_scrolling_total_size) {
            int size =
                    *view_scrolling_total_size < max_chars_per_line ? *view_scrolling_total_size : max_chars_per_line;
            copy(
                    (char *) value,
                    value_start_address + view_scrolling_step,
                    size);
            value[size] = '\0';
        }
    }
    else {
        strcpy(name, "Error");
        strcpy(value, "Out-of-bounds");
    }

    return tokenKeyIndex;
}

//---------------------------------------------

void json_parse(
        parsed_json_t* parsed_json,
        const char* transaction)
{
    jsmn_parser parser;
    jsmn_init(&parser);

    parsed_json->NumberOfTokens = jsmn_parse(
            &parser,
            transaction,
            strlen(transaction),
            parsed_json->Tokens,
            MAX_NUMBER_OF_TOKENS);

    parsed_json->CorrectFormat = false;
    if (parsed_json->NumberOfTokens >= 1
        &&
            parsed_json->Tokens[0].type != JSMN_OBJECT) {

        parsed_json->CorrectFormat = true;
    }
}

int json_validate(
        const char* transaction,
        char* errorMsg,
        int errMsgLength)
{
    strcpy(errorMsg, "Not implemented.");
    // Here we make sure that json have correct format according to the spec
    return -1;
}

int array_get_element_count(int array_token_index,
                            const parsed_json_t* parsed_transaction)
{
    jsmntok_t array_token = parsed_transaction->Tokens[array_token_index];
    int token_index = array_token_index;
    int element_count = 0;
    int prev_element_end = array_token.start;
    while (true) {
        token_index++;
        if (token_index >= parsed_transaction->NumberOfTokens) {
            break;
        }
        jsmntok_t current_token = parsed_transaction->Tokens[token_index];
        if (current_token.start > array_token.end) {
            break;
        }
        if (current_token.start <= prev_element_end) {
            continue;
        }
        prev_element_end = current_token.end;
        element_count++;
    }

    return element_count;
}

int array_get_nth_element(int array_token_index,
                        int element_index,
                        const parsed_json_t* parsed_transaction)
{
    jsmntok_t array_token = parsed_transaction->Tokens[array_token_index];
    int token_index = array_token_index;
    int element_count = 0;
    int prev_element_end = array_token.start;
    while (true) {
        token_index++;
        if (token_index >= parsed_transaction->NumberOfTokens) {
            break;
        }
        jsmntok_t current_token = parsed_transaction->Tokens[token_index];
        if (current_token.start > array_token.end) {
            break;
        }
        if (current_token.start <= prev_element_end) {
            continue;
        }
        prev_element_end = current_token.end;
        if (element_count == element_index) {
            return token_index;
        }
        element_count++;
    }

    return -1;
}

int object_get_element_count(int object_token_index,
                             const parsed_json_t* parsed_transaction)
{
    jsmntok_t object_token = parsed_transaction->Tokens[object_token_index];
    int token_index = object_token_index;
    int element_count = 0;
    int prev_element_end = object_token.start;
    token_index++;
    while (true) {
        if (token_index >= parsed_transaction->NumberOfTokens) {
            break;
        }
        jsmntok_t key_token = parsed_transaction->Tokens[token_index++];
        jsmntok_t value_token = parsed_transaction->Tokens[token_index];
        if (key_token.start > object_token.end) {
            break;
        }
        if (key_token.start <= prev_element_end) {
            continue;
        }
        prev_element_end = value_token.end;
        element_count++;
    }

    return element_count;
}

int object_get_nth_key(int object_token_index,
                     int object_element_index,
                     const parsed_json_t* parsed_transaction)
{
    jsmntok_t object_token = parsed_transaction->Tokens[object_token_index];
    int token_index = object_token_index;
    int element_count = 0;
    int prev_element_end = object_token.start;
    token_index++;
    while (true) {
        if (token_index >= parsed_transaction->NumberOfTokens) {
            break;
        }
        jsmntok_t key_token = parsed_transaction->Tokens[token_index++];
        jsmntok_t value_token = parsed_transaction->Tokens[token_index];
        if (key_token.start > object_token.end) {
            break;
        }
        if (key_token.start <= prev_element_end) {
            continue;
        }
        prev_element_end = value_token.end;
        if (element_count == object_element_index) {
            return token_index-1;
        }
        element_count++;
    }

    return -1;
}

int object_get_nth_value(int object_token_index,
                       int object_element_index,
                       const parsed_json_t* parsed_transaction)
{
    int key_index = object_get_nth_key(object_token_index, object_element_index, parsed_transaction);
    if (key_index != -1) {
        return key_index + 1;
    }
    return -1;
}

int object_get_value(int object_token_index,
                     const char* key_name,
                     const parsed_json_t* parsed_transaction,
                     const char* transaction)
{
    int length = strlen(key_name);
    jsmntok_t object_token = parsed_transaction->Tokens[object_token_index];
    int token_index = object_token_index;
    int prev_element_end = object_token.start;
    token_index++;
    while (true) {
        if (token_index >= parsed_transaction->NumberOfTokens) {
            break;
        }
        jsmntok_t key_token = parsed_transaction->Tokens[token_index++];
        jsmntok_t value_token = parsed_transaction->Tokens[token_index];
        if (key_token.start > object_token.end) {
            break;
        }
        if (key_token.start <= prev_element_end) {
            continue;
        }
        prev_element_end = value_token.end;
        char* cmper = (char*)(transaction + key_token.start);
        if (memcmp(key_name, cmper, length) == 0) {
            return token_index;
        }
    }

    return -1;
}

int display_value(
        char* value,
        int token_index,
        int* current_item_index,
        int item_index_to_display,
        const parsed_json_t* parsed_transaction,
        unsigned int* view_scrolling_total_size, // output
        unsigned int view_scrolling_step, // input
        unsigned int max_chars_per_line, // input
        const char* transaction, // input
        void(*copy)(void* dst, const void* source, unsigned int size)) {

    if (*current_item_index == item_index_to_display) {

        *view_scrolling_total_size =
                parsed_transaction->Tokens[token_index].end - parsed_transaction->Tokens[token_index].start;

        const char* address_ptr = transaction + parsed_transaction->Tokens[token_index].start;
        if (view_scrolling_step < *view_scrolling_total_size) {
            int size =
                    *view_scrolling_total_size < max_chars_per_line ? *view_scrolling_total_size : max_chars_per_line;
            copy(value, address_ptr + view_scrolling_step, size);
            value[size] = '\0';
        }
        return 1;
    }
    *current_item_index = *current_item_index + 1;
    return 0;
}

void display_key(
        char* key,
        int token_index,
        const parsed_json_t* parsed_transaction,
        unsigned int max_chars_per_line, // input
        const char* transaction, // input
        void(*copy)(void* dst, const void* source, unsigned int size))
{
    unsigned int key_size = parsed_transaction->Tokens[token_index].end - parsed_transaction->Tokens[token_index].start;
    const char* address_ptr = transaction + parsed_transaction->Tokens[token_index].start;
    unsigned int size = key_size < max_chars_per_line ? key_size : max_chars_per_line;
    copy(key, address_ptr, size);
    key[size] = '\0';
}

void append_keys(char* key, const char* temp_key)
{
    int size = strlen(key);
    if (size > 0) {
        key[size] = '/';
        size++;
    }
    strcpy(key+size, temp_key);
}

void remove_last(char* key)
{
    int size = strlen(key);
    char* last = key + size;
    while (last > key) {
        if (*last == '/') {
            *last = '\0';
            return;
        }
        last--;
    }
    *last = '\0';
}

int display_get_arbitrary_items(
        int token_index,
        const parsed_json_t* parsed_transaction,
        const char* transaction)
{
    int number_of_items = 0;
    char dummy[1];
    unsigned int dummy_size = 0;
    display_arbitrary_item(
            -1,
            dummy,
            dummy,
            token_index,
            &number_of_items,
            0,
            parsed_transaction,
            &dummy_size,
            0,
            0,
            transaction,
            NULL);
    return number_of_items;
}

int display_arbitrary_item(
        int item_index_to_display, //input
        char* key, // output
        char* value, // output
        int token_index, // input
        int* current_item_index, // input
        int level, // input
        const parsed_json_t* parsed_transaction, // input
        unsigned int* view_scrolling_total_size, // output
        unsigned int view_scrolling_step, // input
        unsigned int max_chars_per_line, // input
        const char* transaction, // input
        void(*copy)(void* dst, const void* source, unsigned int size))
{
//    if level == 2
//    show value as json-encoded string
//    else
//    switch typeof(json) {
//        case object:
//            for (key, value) in object:
//                    show key
//                    display(value, level + 1)
//        case array:
//            for element in array:
//        display(element, level + 1)
//        otherwise:
//        show value as json-encoded string
//    }
    if (level == 2) {
        return display_value(
                value,
                token_index,
                current_item_index,
                item_index_to_display,
                parsed_transaction,
                view_scrolling_total_size,
                view_scrolling_step,
                max_chars_per_line,
                transaction,
                copy);
    }
    else {
        switch (parsed_transaction->Tokens[token_index].type) {
            case JSMN_STRING:
                return display_value(
                        value,
                        token_index,
                        current_item_index,
                        item_index_to_display,
                        parsed_transaction,
                        view_scrolling_total_size,
                        view_scrolling_step,
                        max_chars_per_line,
                        transaction,
                        copy);

            case JSMN_PRIMITIVE:
                return display_value(
                        value,
                        token_index,
                        current_item_index,
                        item_index_to_display,
                        parsed_transaction,
                        view_scrolling_total_size,
                        view_scrolling_step,
                        max_chars_per_line,
                        transaction,
                        copy);

            case JSMN_OBJECT: {
                int el_count = object_get_element_count(token_index, parsed_transaction);
                for (int i = 0; i < el_count; ++i) {
                    int key_index = object_get_nth_key(token_index, i, parsed_transaction);
                    int value_index = object_get_nth_value(token_index, i, parsed_transaction);

                    if (item_index_to_display != -1) {
                        char key_temp[10];
                        display_key(
                                key_temp,
                                key_index,
                                parsed_transaction,
                                max_chars_per_line,
                                transaction,
                                copy);

                        append_keys(key, key_temp);
                    }

                    int found = display_arbitrary_item(
                            item_index_to_display,
                            key,
                            value,
                            value_index,
                            current_item_index,
                            level + 1,
                            parsed_transaction,
                            view_scrolling_total_size,
                            view_scrolling_step,
                            max_chars_per_line,
                            transaction,
                            copy);
                    if (found == 1) {
                        return 1;
                    } else {
                        if (item_index_to_display != -1) {
                            remove_last(key);
                        }
                    }
                }
                break;
            }
            case JSMN_ARRAY: {
                int el_count = array_get_element_count(token_index, parsed_transaction);
                for (int i = 0; i < el_count; ++i) {
                    int element_index = array_get_nth_element(token_index, i, parsed_transaction);
                    int found = display_arbitrary_item(
                            item_index_to_display,
                            key,
                            value,
                            element_index,
                            current_item_index,
                            level,
                            parsed_transaction,
                            view_scrolling_total_size,
                            view_scrolling_step,
                            max_chars_per_line,
                            transaction,
                            copy);
                    if (found == 1) {
                        return 1;
                    }
                }
                break;
            }
            default:
                return 0;
        }
        return 0;
    }
}