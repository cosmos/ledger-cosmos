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
#include <stdio.h>
#include "tx_parser.h"
#include "tx_display.h"
#include "json/json_parser.h"
#include "lib/parser_impl.h"

#if defined(TARGET_NANOS) || defined(TARGET_NANOX)
#include "os.h"
#else
#define PIC(X) X
#endif

// Required pages
// FIXME: the required root items have been moved to a function due to PIC issues. Refactor and fix
const char *get_required_root_item(uint8_t i) {
    switch (i) {
        case 0:
            return "chain_id";
        case 1:
            return "account_number";
        case 2:
            return "sequence";
        case 3:
            return "fee";
        case 4:
            return "memo";
        case 5:
            return "msgs";
        default:
            return "?";
    }
}

//const char *root_required_items[NUM_REQUIRED_ROOT_PAGES] = {
//        "chain_id",
//        "account_number",
//        "sequence",
//        "fee",
//        "memo",
//        "msgs"
//};

static const uint16_t root_max_level[NUM_REQUIRED_ROOT_PAGES] = {
        2, // "chain_id",
        2, // "account_number",
        2, // "sequence",
        1, // "fee",
        2, // "memo"
        2, // "msgs"
};

static const key_subst_t key_substitutions[NUM_KEY_SUBSTITUTIONS] = {
        {"chain_id",                     "Chain ID"},
        {"account_number",               "Account"},
        {"sequence",                     "Sequence"},
        {"memo",                         "Memo"},
        {"fee/amount",                   "Fee"},
        {"fee/gas",                      "Gas"},
        {"msgs/type",                    "Type"},

        // FIXME: Are these obsolete?? multisend?
        {"msgs/inputs/address",          "Source Address"},
        {"msgs/inputs/coins",            "Source Coins"},
        {"msgs/outputs/address",         "Dest Address"},
        {"msgs/outputs/coins",           "Dest Coins"},

        // MsgSend
        {"msgs/value/from_address",             "From"},
        {"msgs/value/to_address",               "To"},
        {"msgs/value/amount",                   "Amount"},

        // MsgDelegate
        {"msgs/value/delegator_address", "Delegator"},
        {"msgs/value/validator_address", "Validator"},

        // MsgUndelegate
//        {"msgs/value/delegator_address", "Delegator"},
//        {"msgs/value/validator_address", "Validator"},

        // MsgBeginRedelegate
//        {"msgs/value/delegator_address", "Delegator"},
        {"msgs/value/validator_src_address", "Validator Source"},
        {"msgs/value/validator_dst_address", "Validator Dest"},

        // MsgSubmitProposal
        {"msgs/value/description",              "Description"},
        {"msgs/value/initial_deposit/amount",  "Deposit Amount"},
        {"msgs/value/initial_deposit/denom",   "Deposit Denom"},
        {"msgs/value/proposal_type",            "Proposal"},
        {"msgs/value/proposer",                 "Proposer"},
        {"msgs/value/title",                    "Title"},

        // MsgDeposit
        {"msgs/value/depositer",                 "Sender"},
        {"msgs/value/proposal_id",               "Proposal ID"},
        {"msgs/value/amount",                    "Amount"},

        // MsgVote
        {"msgs/value/voter",                    "Description"},
//        {"msgs/value/proposal_id",              "Proposal ID"},
        {"msgs/value/option",                   "Option"},

        // MsgWithdrawDelegationReward
//        {"msgs/value/delegator_address", "Delegator"},      // duplicated
//        {"msgs/value/validator_address", "Validator"},      // duplicated
};

static const key_subst_t value_substitutions[NUM_VALUE_SUBSTITUTIONS] = {
        {"cosmos-sdk/MsgSend", "Send"},
        {"cosmos-sdk/MsgDelegate", "Delegate"},
        {"cosmos-sdk/MsgUndelegate", "Undelegate"},
        {"cosmos-sdk/MsgBeginRedelegate", "Redelegate"},
        {"cosmos-sdk/MsgSubmitProposal", "Propose"},
        {"cosmos-sdk/MsgDeposit", "Deposit"},
        {"cosmos-sdk/MsgVote", "Vote"},
        {"cosmos-sdk/MsgWithdrawDelegationReward", "Withdraw Reward"},
};

#define STRNCPY_S(DST, SRC, DST_SIZE) \
    strncpy(DST, SRC, DST_SIZE - 1); \
    DST[DST_SIZE - 1] = 0;

display_cache_t display_cache;

display_cache_t *tx_display_cache() {
    return &display_cache;
}

void tx_display_index_root() {
    if (parser_tx_obj.cache_valid) {
        return;
    }

    // Clear values
    display_cache.numItems = 0;
    memset(display_cache.num_subpages, 0, NUM_REQUIRED_ROOT_PAGES);
    memset(display_cache.subroot_start_token, TX_TOKEN_NOT_FOUND, NUM_REQUIRED_ROOT_PAGES);

    // Calculate pages
    int8_t found = 0;
    for (int8_t idx = 0; idx < NUM_REQUIRED_ROOT_PAGES; idx++) {
        const int16_t subroot_token_idx = object_get_value(ROOT_TOKEN_INDEX,
                                                           get_required_root_item(idx),
                                                           &parser_tx_obj.json,
                                                           parser_tx_obj.tx);
        if (subroot_token_idx < 0) {
            break;
        }

        display_cache.num_subpages[idx] = 0;
        display_cache.subroot_start_token[idx] = subroot_token_idx;

        char tmp_key[2];
        char tmp_val[2];
        INIT_QUERY_CONTEXT(tmp_key, sizeof(tmp_key), tmp_val, sizeof(tmp_val), 0, root_max_level[idx])
        STRNCPY_S(parser_tx_obj.tx_ctx.query.out_key, get_required_root_item(idx), parser_tx_obj.tx_ctx.query.out_key_len);
        parser_tx_obj.tx_ctx.max_depth = MAX_RECURSION_DEPTH;
        parser_tx_obj.tx_ctx.query.item_index = 0;

        found = 0;
        while (found >= 0) {
            parser_tx_obj.tx_ctx.item_index_current = 0;
            found = tx_traverse(subroot_token_idx);

            if (found >= 0) {
                display_cache.num_subpages[idx]++;
                parser_tx_obj.tx_ctx.query.item_index++;
            }
        };
        display_cache.numItems += display_cache.num_subpages[idx];

        if (display_cache.num_subpages[idx] == 0) {
            break;
        }
    }

    parser_tx_obj.cache_valid = 1;
}

int16_t tx_display_numItems() {
    tx_display_index_root();
    return display_cache.numItems;
}

// This function assumes that the tx_ctx has been set properly
int16_t tx_display_get_item(uint16_t itemIndex) {
    if (!parser_tx_obj.cache_valid) {
        return ERR_MUST_INDEX_FIRST;
    }

    // TODO: Verify it has been properly set?
    parser_tx_obj.tx_ctx.query.out_key[0] = 0;
    parser_tx_obj.tx_ctx.query.out_val[0] = 0;
    if (itemIndex < 0 || itemIndex >= display_cache.numItems) {
        return -1;
    }

    parser_tx_obj.tx_ctx.query.item_index = 0;
    uint16_t root_index = 0;
    for (uint16_t i = 0; i < itemIndex; i++) {
        parser_tx_obj.tx_ctx.query.item_index++;
        if (parser_tx_obj.tx_ctx.query.item_index >= display_cache.num_subpages[root_index]) {
            parser_tx_obj.tx_ctx.query.item_index = 0;
            root_index++;
        }
    }

    parser_tx_obj.tx_ctx.item_index_current = 0;
    parser_tx_obj.tx_ctx.max_level = root_max_level[root_index];
    parser_tx_obj.tx_ctx.max_depth = MAX_RECURSION_DEPTH;

    STRNCPY_S(parser_tx_obj.tx_ctx.query.out_key,
        get_required_root_item(root_index),
        parser_tx_obj.tx_ctx.query.out_key_len);

    int16_t ret = tx_traverse(display_cache.subroot_start_token[root_index]);

    return ret;
}

void tx_display_make_friendly() {
    // post process keys
    for (int8_t i = 0; i < NUM_KEY_SUBSTITUTIONS; i++) {
        if (!strcmp(parser_tx_obj.tx_ctx.query.out_key, key_substitutions[i].str1)) {
            STRNCPY_S(parser_tx_obj.tx_ctx.query.out_key,
                      key_substitutions[i].str2,
                      parser_tx_obj.tx_ctx.query.out_key_len);
            break;
        }
    }

    for (int8_t i = 0; i < NUM_VALUE_SUBSTITUTIONS; i++) {
        if (!strcmp(parser_tx_obj.tx_ctx.query.out_val, value_substitutions[i].str1)) {
            STRNCPY_S(parser_tx_obj.tx_ctx.query.out_val,
                      value_substitutions[i].str2,
                      parser_tx_obj.tx_ctx.query.out_val_len);
            break;
        }
    }

}

