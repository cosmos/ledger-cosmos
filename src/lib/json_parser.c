/*******************************************************************************
*   (c) ZondaX GmbH
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

#if defined(TARGET_NANOS) || defined(TARGET_NANOX)
    #include "os.h"
    #define EQUALS(_P, _Q, _LEN) (os_memcmp( PIC(_P), PIC(_Q), (_LEN))==0)
#else
    #define EQUALS(_P, _Q, _LEN) (memcmp( (_P), (_Q), (_LEN))==0)
#endif

void reset_parsed_json(parsed_json_t *parser_data) {
    memset(parser_data, 0, sizeof(parsed_json_t));
}

const char *json_parse(parsed_json_t *parsed_json, const char *transaction) {
    return json_parse_s(parsed_json, transaction, strlen(transaction));

}

const char *json_parse_s(parsed_json_t *parsed_json,
                         const char *transaction,
                         uint16_t transaction_length) {
    jsmn_parser parser;
    jsmn_init(&parser);

    reset_parsed_json(parsed_json);

    int num_tokens = jsmn_parse(
            &parser,
            transaction,
            transaction_length,
            parsed_json->Tokens,
            MAX_NUMBER_OF_TOKENS);

    switch (num_tokens) {
        case JSMN_ERROR_NOMEM:
            return "NOMEM: JSON string contains too many tokens";
        case JSMN_ERROR_INVAL:
            return "Invalid character in JSON string";
        case JSMN_ERROR_PART:
            return "JSON string is not complete";
    }

    parsed_json->NumberOfTokens = 0;
    parsed_json->IsValid = 0;

    // Parsing error
    if (num_tokens <= 0) {
        return "Unknown parser error";
    }

    // We cannot support if number of tokens exceeds the limit
    if (num_tokens > MAX_NUMBER_OF_TOKENS) {
        return "TOK: JSON string contains too many tokens";
    }

    parsed_json->NumberOfTokens = num_tokens;
    parsed_json->IsValid = true;
    return NULL;
}

uint16_t array_get_element_count(uint16_t array_token_index,
                                 const parsed_json_t *parsed_transaction) {
    if (array_token_index < 0 || array_token_index > parsed_transaction->NumberOfTokens) {
        return 0;
    }

    jsmntok_t array_token = parsed_transaction->Tokens[array_token_index];
    uint16_t token_index = array_token_index;
    uint16_t element_count = 0;
    uint16_t prev_element_end = array_token.start;
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

int16_t array_get_nth_element(uint16_t array_token_index,
                              uint16_t element_index,
                              const parsed_json_t *parsed_transaction) {
    if (array_token_index < 0 || array_token_index > parsed_transaction->NumberOfTokens) {
        return -1;
    }

    jsmntok_t array_token = parsed_transaction->Tokens[array_token_index];
    uint16_t token_index = array_token_index;
    uint16_t element_count = 0;
    uint16_t prev_element_end = array_token.start;
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

uint16_t object_get_element_count(uint16_t object_token_index,
                                  const parsed_json_t *parsed_transaction) {
    if (object_token_index < 0 || object_token_index > parsed_transaction->NumberOfTokens) {
        return 0;
    }

    jsmntok_t object_token = parsed_transaction->Tokens[object_token_index];
    uint16_t token_index = object_token_index;
    uint16_t element_count = 0;
    uint16_t prev_element_end = object_token.start;
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

int16_t object_get_nth_key(uint16_t object_token_index,
                           uint16_t object_element_index,
                           const parsed_json_t *parsed_transaction) {
    if (object_token_index < 0 || object_token_index > parsed_transaction->NumberOfTokens) {
        return -1;
    }

    jsmntok_t object_token = parsed_transaction->Tokens[object_token_index];
    uint16_t token_index = object_token_index;
    uint16_t element_count = 0;
    uint16_t prev_element_end = object_token.start;
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

int16_t object_get_nth_value(uint16_t object_token_index,
                             uint16_t object_element_index,
                             const parsed_json_t *parsed_transaction) {
    if (object_token_index < 0 || object_token_index > parsed_transaction->NumberOfTokens) {
        return -1;
    }

    int key_index = object_get_nth_key(object_token_index, object_element_index, parsed_transaction);
    if (key_index != -1) {
        return key_index + 1;
    }
    return -1;
}

int16_t object_get_value(uint16_t object_token_index,
                         const char *key_name,
                         const parsed_json_t *parsed_transaction,
                         const char *transaction) {
    if (object_token_index < 0 || object_token_index > parsed_transaction->NumberOfTokens) {
        return -1;
    }

    const jsmntok_t object_token = parsed_transaction->Tokens[object_token_index];

    int token_index = object_token_index;
    int prev_element_end = object_token.start;
    token_index++;

    while (token_index < parsed_transaction->NumberOfTokens) {
        const jsmntok_t key_token = parsed_transaction->Tokens[token_index];
        token_index++;
        const jsmntok_t value_token = parsed_transaction->Tokens[token_index];

        if (key_token.start > object_token.end) {
            break;
        }
        if (key_token.start <= prev_element_end) {
            continue;
        }
        prev_element_end = value_token.end;

        if ( ((uint16_t) strlen(key_name)) == (key_token.end - key_token.start)) {
            if (EQUALS(key_name, transaction + key_token.start, key_token.end - key_token.start)) {
                return token_index;
            }
        }
    }

    return -1;
}
