/* mbed Microcontroller Library
 * Copyright (c) 2019 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
 */
#include "mbed.h"
#include "keypad.h"
#include "ThisThread.h"
#include "SLCD.h"
#include "FlashIAP.h"

#define FLASH_TOTAL_SIZE    0x00040000  // 256KB
#define FLASH_BLOCK_SIZE    1024        // 扇区大小1KB
#define PAGE_SIZE           256         
#define SECTORS             2
#define PAGES_PER_SECTOR    4
#define DEFAULT_ADMIN_PW    "00000000"
#define MAX_USERS           9


#define SECTOR0_START       (FLASH_TOTAL_SIZE - 2 * FLASH_BLOCK_SIZE)  // 0x3F800
#define SECTOR1_START       (FLASH_TOTAL_SIZE - FLASH_BLOCK_SIZE)       // 0x3FC00


struct PasswordEntry 
{
    uint32_t index;
    char passwords[10][9];
    uint32_t checksum;
};

Keypad mykeypad(PTC8, PTA5, PTA4, PTA12, PTD3, PTA2, PTA1);
SLCD mydisplay;
FlashIAP mymemory;

const int bufferSize = 32;
const int displayDigits = 4;
char keypad_input[bufferSize] = {0};
char display_output[bufferSize] = {0};
int input_index = 0;
int output_index = 0;

PasswordEntry current_entry;
uint8_t input_pos = 0;
uint32_t current_sector = SECTOR0_START;
uint32_t current_page = 0;
char input_buffer[9] = {0};


void init_flash();
uint32_t calculate_checksum(const PasswordEntry& entry);
bool load_latest_entry();
void save_entry();
void Input(char key);
void handle_input_password(char new_char);
void Output(const char* output_str);
//void validate_password(const char* pw);


int main() 
{
    mydisplay.clear();
    mydisplay.Home();

    init_flash();

    for (int i = 0; i < bufferSize; i++) 
    {
        keypad_input[i] = ' ';
    }

    while(true)
     {
        char key = mykeypad.ReadKey();

        if(key != 0) 
        {
            Input(key);
        }
        ThisThread::sleep_for(200ms);
    }
}

void init_flash() 
{
    mymemory.init();
    if(!load_latest_entry()) 
    {
        memset(&current_entry, 0, sizeof(PasswordEntry));
        strcpy(current_entry.passwords[0], DEFAULT_ADMIN_PW);
        current_entry.index = 1;
        current_entry.checksum = calculate_checksum(current_entry);
        save_entry();
        Output(" INIT DEFAULT");
    }
}

uint32_t calculate_checksum(const PasswordEntry& entry) 
{
    uint32_t crc = 0xFFFFFFFF;
    //const uint8_t* data = (uint8_t*)&current_entry;
    const uint8_t* data = reinterpret_cast<const uint8_t*>(&entry);
    const size_t data_size = sizeof(PasswordEntry) - sizeof(uint32_t);
    for(size_t i = 0; i < data_size; i++) 
    {
        crc ^= (data[i] << 24);
        for(int j = 0; j < 8; j++) 
        {
            crc = (crc << 1) ^ ((crc & 0x80000000) ? 0x04C11DB7 : 0);
        }
    }
    return crc;
}

bool load_latest_entry() 
{
    uint32_t max_index = 0;
    PasswordEntry valid_entry;
    
    for(int s = 0; s < SECTORS; s++) 
    {
        uint32_t sector = (s == 0) ? (FLASH_TOTAL_SIZE - 2*FLASH_BLOCK_SIZE) 
                                  : (FLASH_TOTAL_SIZE - FLASH_BLOCK_SIZE);
        
        for(int p = PAGES_PER_SECTOR -1; p >=0; p--) 
        {
            uint32_t addr = sector + p*PAGE_SIZE;
            PasswordEntry temp;
            mymemory.read(&temp, addr, sizeof(PasswordEntry));
            
            uint32_t stored_checksum = temp.checksum;
            temp.checksum = 0;
            uint32_t calc_checksum = calculate_checksum(temp);
            
            if(stored_checksum == calc_checksum && temp.index > max_index) 
            {
                max_index = temp.index;
                memcpy(&valid_entry, &temp, sizeof(PasswordEntry));
                valid_entry.checksum = stored_checksum;
            }
        }
    }
    
    if(max_index > 0) 
    {
        memcpy(&current_entry, &valid_entry, sizeof(PasswordEntry));
        return true;
    }
    return false;
}

void save_entry() 
{
    current_entry.index++;
    current_entry.checksum = calculate_checksum(current_entry);
    
    uint32_t addr = current_sector + (current_page * PAGE_SIZE);
    if(current_page % PAGES_PER_SECTOR == 0) 
    {
        mymemory.erase(current_sector, FLASH_BLOCK_SIZE);
    }

    char memory_buffer[PAGE_SIZE];
    memcpy(memory_buffer, &current_entry, sizeof(PasswordEntry));
    
    if(mymemory.program(memory_buffer, addr, PAGE_SIZE) == 0) 
    {
        current_page = (current_page + 1) % PAGES_PER_SECTOR;
        if(current_page == 0) 
        {
            current_sector = (current_sector == (FLASH_TOTAL_SIZE - 2*FLASH_BLOCK_SIZE)) 
                            ? (FLASH_TOTAL_SIZE - FLASH_BLOCK_SIZE) 
                            : (FLASH_TOTAL_SIZE - 2*FLASH_BLOCK_SIZE);
        }
    }
}



void Input(char key)
{
    if(key == '#') 
    {  
        memset(keypad_input, 0, bufferSize);
        input_index = 0;
        input_pos = 0;
        mydisplay.clear();
    }
    else if(key == '*') 
    {  
        if(input_pos == 8) 
        {
            //validate_password(input_buffer);
             mydisplay.clear();
        }
    }
    else if(key >= '0' && key <= '9') 
    {  
        if(input_pos < 8) 
        {
            handle_input_password(key);
        }
    }
}


void handle_input_password(char new_char) 
{
    if(new_char >= '0' && new_char <= '9') 
    {
        keypad_input[input_index] = new_char;
        input_index = (input_index + 1) % bufferSize;

        input_buffer[input_pos++] = new_char;  //check

        mydisplay.clear();
        mydisplay.Home();
            
        int input_start = (input_index - displayDigits + bufferSize) % bufferSize;
            
        for (int i = displayDigits - 1; i >= 0; i--) 
        {
            char c = keypad_input[(input_start + i) % bufferSize];
            mydisplay.putc(c);
        }
    }
}


void Output(const char* output_str) 
{
    int print_len = strlen(output_str);
    int max_print = bufferSize - 1; 
    char SLCD_output[4] = {' ',' ',' ',' '};
    int output_scrollPosition = 0;
    int output_start;
    int output_start_max = print_len - displayDigits;
    int d;
    int end_display;

    end_display = print_len * 3 - displayDigits + 1;

    for (int b = 0; b < bufferSize; b++) 
    {
        display_output[b] = ' ';
    }

    output_index = 0;
    
    for (int o = 0; output_str[o] != '\0'; ++o) 
    {
        display_output[output_index] = output_str[o];
        output_index = (output_index + 1) % bufferSize;
    }
    display_output[print_len] = '\0';
    
    output_start = 0;

    d = 0;
    while (true)
    { 
        mydisplay.clear();
        mydisplay.Home();
        for (int s = 0; s < displayDigits; s++) 
        {
            int char_idx = (output_start + s) % print_len; 
            SLCD_output[s] = output_str[char_idx];
        }
        mydisplay.puts(SLCD_output);
        
        output_start = (output_start + 1) % print_len;
        ThisThread::sleep_for(300ms);
        d++;
        if(d >= end_display)
        {
            mydisplay.clear();
            mydisplay.Home();
            break;
        }
    }
}