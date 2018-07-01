/*******************************************************************************
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

#include "json_parser.h"

void json_parse(parsed_json_t *parsed_json, const char *transaction) {
    jsmn_parser parser;
    jsmn_init(&parser);

    int num_tokens = jsmn_parse(
            &parser,
            transaction,
            strlen(transaction),
            parsed_json->Tokens,
            MAX_NUMBER_OF_TOKENS);

    parsed_json->CorrectFormat = false;

    if (num_tokens > 255) {
        return;
    }

    parsed_json->NumberOfTokens = (byte) num_tokens;
    if (parsed_json->NumberOfTokens >= 1 &&
        parsed_json->Tokens[0].type != JSMN_OBJECT) {
        parsed_json->CorrectFormat = true;
    }
}

int json_validate(const char *transaction,
                  char *errorMsg,
                  int errMsgLength) {
    strcpy(errorMsg, "Not implemented.");
    // Here we make sure that json have correct format according to the spec
    return -1;
}

int array_get_element_count(int array_token_index,
                            const parsed_json_t *parsed_transaction) {
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
                          const parsed_json_t *parsed_transaction) {
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
                             const parsed_json_t *parsed_transaction) {
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
                       const parsed_json_t *parsed_transaction) {
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
            return token_index - 1;
        }
        element_count++;
    }

    return -1;
}

int object_get_nth_value(int object_token_index,
                         int object_element_index,
                         const parsed_json_t *parsed_transaction) {
    int key_index = object_get_nth_key(object_token_index, object_element_index, parsed_transaction);
    if (key_index != -1) {
        return key_index + 1;
    }
    return -1;
}

int object_get_value(int object_token_index,
                     const char *key_name,
                     const parsed_json_t *parsed_transaction,
                     const char *transaction) {
    size_t length = strlen(key_name);
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
        char *cmper = (char *) (transaction + key_token.start);
        if (memcmp(key_name, cmper, length) == 0) {
            return token_index;
        }
    }

    return -1;
}