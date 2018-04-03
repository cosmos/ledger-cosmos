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
#include "os.h"

// Clears last signature and resets derivation path to the default value
void signature_reset();

// Sets the derivation path used to retrieve private key
void signature_set_derivation_path(uint32_t* path, uint32_t path_size);

// Creates signature of the message using (...TODO)
// @return
// *   1 if signature is verified
// *   0 is signature is not verified
int signature_create(
        uint8_t* message,
        uint16_t message_length);

// Returns last created signature
uint8_t* signature_get();

// Returns last created signature's length
uint32_t signature_length();