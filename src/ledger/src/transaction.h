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
#pragma once
#include "json_parser.h"
#include "os.h"

// Clears the transaction buffer
void transaction_reset();

// Appends buffer to the end of the current transaction buffer
// Transaction buffer will grow until it reaches the maximum allowed size
void transaction_append(
        unsigned char* buffer,
        uint32_t length);

// Returns size of the raw json transaction buffer
uint32_t transaction_get_buffer_length();

// Returns the raw json transaction buffer
uint8_t* transaction_get_buffer();

// Parse json message stored in transaction buffer
// This function should be called as soon as full buffer data is loaded.
void transaction_parse();

// Returns parsed representation of the transaction message
parsed_json_t* transaction_get_parsed();