#pragma once

#include "JsonParser.h"

#define CLA 0x80
#define OFFSET_CLA       0
#define OFFSET_INS       1  //< Instruction offset
#define OFFSET_PCK_INDEX 2  //< Package index offset
#define OFFSET_PCK_COUNT 3  //< Package count offset
#define OFFSET_DATA      4  //< Data offset

extern parsed_json_t parsed_json;
extern char json_buffer[1000];

extern uint32_t json_buffer_write_pos;
extern uint32_t json_buffer_size;

extern volatile uint32_t stackStartAddress;

void app_init();

void app_main();