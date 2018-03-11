#pragma once

#include "JsonParser.h"

#define CLA 0x80
#define OFFSET_CLA 0
#define OFFSET_INS 1

extern parsed_json_t parsed_json;

extern volatile uint32_t stackStartAddress;
extern volatile uint32_t maxUsedStackSize;

void app_init();

void app_main();