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
        void(*copy)(void* dst, const void* source, unsigned int size));


// The main function that parses jsonString and creates a parsed message
// which contains all the json tokens plus skeleton of SendMsg with links to those tokens
void ParseSignedMsg(parsed_json_t* parsedMessage, const char* signedMsg);

// Returns number of key/value elements in the sdk.StdSignMsg
int SignedMsgGetNumberOfElements(const parsed_json_t* parsed_message,
                                 const char* message);

// Get key/value pairs (name/value) from the sdk.StdSignMsg based on the
// index of the key/value pair that will be displayed
int SignedMsgGetInfo(
        char *name,
        char *value,
        int index,
        const parsed_json_t* parsed_message,
        unsigned int* view_scrolling_total_size,
        unsigned int view_scrolling_step,
        unsigned int max_chars_per_line,
        const char* message,
        void(*copy)(void* dst, const void* source, unsigned int size));

//---------------------------------------------
// NEW JSON PARSER CODE

// Parse json to create a token representation
void json_parse(
        parsed_json_t* parsed_json,
        const char* transaction);

// Get number of elements in array
int array_get_element_count(
        int array_token_index,
        const parsed_json_t* parsed_transaction);

// Get token index of the nth array's element
int array_get_nth_element(
        int array_token_index,
        int element_index,
        const parsed_json_t* parsed_transaction);

// Get number of elements (key/value pairs) in object
int object_get_element_count(
        int object_token_index,
        const parsed_json_t* parsed_transaction);

// Get token index for the nth key
int object_get_nth_key(
        int object_token_index,
        int object_element_index,
        const parsed_json_t* parsed_transaction);

// Get token index for the nth value
int object_get_nth_value(
        int object_token_index,
        int object_element_index,
        const parsed_json_t* parsed_transaction);

// Get token index for the value that matched given key
int object_get_value(
        int object_token_index,
        const char* key_name,
        const parsed_json_t* parsed_transaction,
        const char* transaction);

// Update value characters from json transaction read from the token_index element.
// Value is only updated if current_item_index (which is incremented internally) matches item_index_to_display
// If value is updated, we also update view_scrolling_total_size to value string length.
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
        void(*copy)(void* dst, const void* source, unsigned int size));

// Update key characters from json transaction read from the token_index element.
void display_key(
        char* key,
        int token_index,
        const parsed_json_t* parsed_transaction,
        unsigned int max_chars_per_line, // input
        const char* transaction, // input
        void(*copy)(void* dst, const void* source, unsigned int size));

// Generic function to display arbitrary json based on the specification
// TODO: This function is currently untested.
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
        void(*copy)(void* dst, const void* source, unsigned int size));

#ifdef __cplusplus
}
#endif
#endif //CI_TEST_JSONPARSER_H
