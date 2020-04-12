/*******************************************************************************
*   (c) 2018, 2019 Zondax GmbH
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

#include "tx_display.h"
#include "tx_parser.h"
#include "parser_impl.h"
#include <zxmacros.h>

#define NUM_REQUIRED_ROOT_PAGES 6

const char *get_required_root_item(root_item_e i) {
    switch (i) {
        case root_item_chain_id:
            return "chain_id";
        case root_item_account_number:
            return "account_number";
        case root_item_sequence:
            return "sequence";
        case root_item_fee:
            return "fee";
        case root_item_memo:
            return "memo";
        case root_item_msgs:
            return "msgs";
        default:
            return "?";
    }
}

static const uint8_t root_max_level[NUM_REQUIRED_ROOT_PAGES] = {
        2, // "chain_id",
        2, // "account_number",
        2, // "sequence",
        1, // "fee",
        2, // "memo"
        2, // "msgs"
};

typedef struct {
    uint16_t total_item_count;

    uint8_t root_item_start_valid[NUM_REQUIRED_ROOT_PAGES];
    // token where the root_item starts (negative for non-existing)
    uint16_t root_item_start_token_idx[NUM_REQUIRED_ROOT_PAGES];
    // number of items the root_item contains
    uint8_t root_item_number_subitems[NUM_REQUIRED_ROOT_PAGES];
} display_cache_t;

display_cache_t display_cache;

parser_error_t tx_display_readTx(parser_context_t *ctx, const uint8_t *data, size_t dataLen) {
    CHECK_PARSER_ERR(parser_init(ctx, data, dataLen))
    CHECK_PARSER_ERR(_readTx(ctx, &parser_tx_obj))
    return parser_ok;
}

parser_error_t tx_indexRootFields() {
    if (parser_tx_obj.flags.cache_valid) {
        return parser_ok;
    }

    // Clear cache
    MEMZERO(&display_cache, sizeof(display_cache_t));
    char tmp_key[20];
    char tmp_val[40];
    char tmp_val_ref[40];
    MEMZERO(&tmp_key, sizeof(tmp_key));
    MEMZERO(&tmp_val, sizeof(tmp_val));
    MEMZERO(&tmp_val_ref, sizeof(tmp_val_ref));

    parser_tx_obj.flags.msg_type_grouping = 1;
    parser_tx_obj.filter_msg_type_count = 0;

    for (int8_t root_item_idx = 0; root_item_idx < NUM_REQUIRED_ROOT_PAGES; root_item_idx++) {
        const char *req_root_item_key = get_required_root_item((root_item_e) root_item_idx);

        uint16_t req_root_item_key_token_idx = 0;

        parser_error_t err = object_get_value(
                &parser_tx_obj.json,
                ROOT_TOKEN_INDEX,
                req_root_item_key,
                &req_root_item_key_token_idx);

        if (err == parser_no_data ) {
            continue;
        }
        CHECK_PARSER_ERR(err)

        // Remember root item start token
        display_cache.root_item_start_valid[root_item_idx] = 1;
        display_cache.root_item_start_token_idx[root_item_idx] = req_root_item_key_token_idx;

        // Now count how many items can be found in this root item
        int32_t current_item_idx = 0;
        while (err == parser_ok) {
            INIT_QUERY_CONTEXT(tmp_key, sizeof(tmp_key),
                               tmp_val, sizeof(tmp_val),
                               0, root_max_level[root_item_idx])
            parser_tx_obj.query.item_index = current_item_idx;
            strncpy_s(parser_tx_obj.query.out_key, req_root_item_key, parser_tx_obj.query.out_key_len);

            uint16_t ret_value_token_index;

            err = tx_traverse_find(
                    display_cache.root_item_start_token_idx[root_item_idx],
                    &ret_value_token_index);

            if (err != parser_ok) {
                continue;
            }

            uint8_t pageCount;
            CHECK_PARSER_ERR(tx_getToken(
                    ret_value_token_index,
                    parser_tx_obj.query.out_val,
                    parser_tx_obj.query.out_key_len,
                    0, &pageCount))

            if (root_item_idx == root_item_memo) {
                if (strlen(parser_tx_obj.query.out_val) == 0) {
                    err = parser_query_no_results;
                    continue;
                }
            }

            if (root_item_idx == root_item_msgs && parser_tx_obj.flags.msg_type_grouping == 1u) {
                if (strcmp(tmp_key, "msgs/type") == 0) {
                    if (parser_tx_obj.filter_msg_type_count == 0) {
                        // First message, initialize expected type
                        strcpy(tmp_val_ref, tmp_val);
                        parser_tx_obj.filter_msg_type_valid_idx = current_item_idx;
                    }

                    if (strcmp(tmp_val_ref, tmp_val) != 0) {
                        // different values, so disable grouping
                        parser_tx_obj.flags.msg_type_grouping = 0;
                        parser_tx_obj.filter_msg_type_count = 0;
                    }

                    parser_tx_obj.filter_msg_type_count++;
                }
            }

            display_cache.root_item_number_subitems[root_item_idx]++;
            current_item_idx++;
        }

        if (err != parser_query_no_results && err != parser_no_data) {
            return err;
        }

        display_cache.total_item_count += display_cache.root_item_number_subitems[root_item_idx];
    }

    parser_tx_obj.flags.cache_valid = 1;

    return parser_ok;
}

parser_error_t tx_display_numItems(uint16_t *num_items) {
    CHECK_PARSER_ERR(tx_indexRootFields())

    *num_items = display_cache.total_item_count;
    // Remove grouped items from list
    if (parser_tx_obj.flags.msg_type_grouping == 1u && parser_tx_obj.filter_msg_type_count > 0) {
        *num_items += 1; // we leave main type
        *num_items -= parser_tx_obj.filter_msg_type_count;
    }

    return parser_ok;
}

// This function assumes that the tx_ctx has been set properly
parser_error_t tx_display_query(uint16_t displayIdx,
                                char *outKey, uint16_t outKeyLen,
                                uint16_t *ret_value_token_index) {
    CHECK_PARSER_ERR(tx_indexRootFields())

    if (displayIdx < 0 || displayIdx >= display_cache.total_item_count) {
        return parser_display_idx_out_of_range;
    }

    uint16_t current_item_index = 0;
    uint16_t root_index = 0;

    // Find root index | display idx -> item_index
    // consume indexed subpages until we get the item index in the subpage
    for (uint16_t i = 0; i < displayIdx; i++) {
        current_item_index++;
        if (current_item_index >= display_cache.root_item_number_subitems[root_index]) {
            current_item_index = 0;

            // Advance root index and skip empty items
            root_index++;
            while (display_cache.root_item_number_subitems[root_index] == 0) root_index++;
        }
    }

    // Prepare query
    char tmp_val[2];
    INIT_QUERY_CONTEXT(outKey, outKeyLen, tmp_val, sizeof(tmp_val),
                       0, root_max_level[root_index])
    parser_tx_obj.query.item_index = current_item_index;
    parser_tx_obj.query._item_index_current = 0;
    parser_tx_obj.query.max_level = root_max_level[root_index];

    strncpy_s(outKey, get_required_root_item((root_item_e) root_index), outKeyLen);

    if (!display_cache.root_item_start_valid[root_index]) {
        return parser_no_data;
    }

    CHECK_PARSER_ERR(tx_traverse_find(
            display_cache.root_item_start_token_idx[root_index],
            ret_value_token_index))

    return parser_ok;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////

static const key_subst_t key_substitutions[] = {
        {"chain_id",                          "Chain ID"},
        {"account_number",                    "Account"},
        {"sequence",                          "Sequence"},
        {"memo",                              "Memo"},
        {"fee/amount",                        "Fee"},
        {"fee/gas",                           "Gas"},
        {"msgs/type",                         "Type"},

        // FIXME: Are these obsolete?? multisend?
        {"msgs/inputs/address",               "Source Address"},
        {"msgs/inputs/coins",                 "Source Coins"},
        {"msgs/outputs/address",              "Dest Address"},
        {"msgs/outputs/coins",                "Dest Coins"},

        // MsgSend
        {"msgs/value/from_address",           "From"},
        {"msgs/value/to_address",             "To"},
        {"msgs/value/amount",                 "Amount"},

        // MsgDelegate
        {"msgs/value/delegator_address",      "Delegator"},
        {"msgs/value/validator_address",      "Validator"},

        // MsgUndelegate
//        {"msgs/value/delegator_address", "Delegator"},
//        {"msgs/value/validator_address", "Validator"},

        // MsgBeginRedelegate
//        {"msgs/value/delegator_address", "Delegator"},
        {"msgs/value/validator_src_address",  "Validator Source"},
        {"msgs/value/validator_dst_address",  "Validator Dest"},

        // MsgSubmitProposal
        {"msgs/value/description",            "Description"},
        {"msgs/value/initial_deposit/amount", "Deposit Amount"},
        {"msgs/value/initial_deposit/denom",  "Deposit Denom"},
        {"msgs/value/proposal_type",          "Proposal"},
        {"msgs/value/proposer",               "Proposer"},
        {"msgs/value/title",                  "Title"},

        // MsgDeposit
        {"msgs/value/depositer",              "Sender"},
        {"msgs/value/proposal_id",            "Proposal ID"},
        {"msgs/value/amount",                 "Amount"},

        // MsgVote
        {"msgs/value/voter",                  "Description"},
//        {"msgs/value/proposal_id",              "Proposal ID"},
        {"msgs/value/option",                 "Option"},

        // MsgWithdrawDelegationReward
//        {"msgs/value/delegator_address", "Delegator"},      // duplicated
//        {"msgs/value/validator_address", "Validator"},      // duplicated
};

parser_error_t tx_display_make_friendly() {
    CHECK_PARSER_ERR(tx_indexRootFields())

    // post process keys
    for (size_t i = 0; i < array_length(key_substitutions); i++) {
        if (!strcmp(parser_tx_obj.query.out_key, key_substitutions[i].str1)) {
            strncpy_s(parser_tx_obj.query.out_key, key_substitutions[i].str2, parser_tx_obj.query.out_key_len);
            break;
        }
    }

    return parser_ok;
}

