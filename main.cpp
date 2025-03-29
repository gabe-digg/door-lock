/* mbed Microcontroller Library
 * Copyright (c) 2019 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
 */
#include "mbed.h"
#include "keypad.h"
#include "ThisThread.h"
#include "SLCD.h"
#include "FlashIAP.h"
#include "Password.h"
#include <string>

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
Password pass;
Thread thread1;
const int bufferSize = 32;
const int displayDigits = 4;
char keypad_input[bufferSize] = {0};
char display_output[bufferSize] = {0};
int input_index = 0;
int output_index = 0;
//uint32_t FLASH_BASE = 0; //用于存储密码的 flash 区域起始地址 Start address of the flash area where the password is stored

PasswordEntry current_entry;
uint8_t input_pos = 0;
uint32_t current_sector = SECTOR0_START;
uint32_t current_page = 0;
char input_buffer[9] = {0};


void init_flash();
uint32_t calculate_checksum(const PasswordEntry& entry);
bool load_latest_entry();
void save_entry();
//void Input(char key);
void handle_input_display(char new_char);
void Output(const char* output_str);
char* keypress_to_array(char one_key);
void keypad_thread ();
//void validate_password(const char* pw);


int main() 
{
    mydisplay.clear();
    mydisplay.Home();

    init_flash();
thread1.start(keypad_thread);   
    for (int i = 0; i < bufferSize; i++) 
    {
        keypad_input[i] = ' ';
    }

}

char* keypress_to_array(char one_key){
    char pressedkey;        
    static char input[9]; // 8 digits + '#' + null terminator
    input[0]= one_key;
    int index = 1;

    while (index < 8)
    {
        pressedkey = mykeypad.ReadKey();
        if ((pressedkey == '#')||(index>=8)) //  ente
        { 
            input[index] = '\0'; // Null-terminate the password
        //printf("Password entered: %s\n", input);
            index = 0; // Reset for next input
            return input;
        } 
        else if (pressedkey >= '0' && pressedkey <= '9') {
            input[index++] = pressedkey;
            handle_input_display(pressedkey);
        }
    ThisThread::sleep_for(10ms);
    }
input[8] = '\0'; // Ensure null termination
return input;
}
void keypad_thread (){
    while(true)
     {
        char key = mykeypad.ReadKey();

        if(key != NO_KEY && key >= '0' && key <= '9') 
        {
            char* password_input = keypress_to_array(key);
            int password_number = pass.check_password(password_input);
    
        }
        //ThisThread::sleep_for(200ms);
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
/*
uint16_t calculate_checksum(const char* data, size_t len)  //计算给定数据的校验和 用指针是为了遍历数据 data的数据类型位char 但在加法时会自动转换为int不影响
{
    uint16_t sum = 0;
    for (size_t i = 0; i < len; i++) {
        sum += data[i];
    }
    return sum;
}

bool boot_load_passwords(Password& pass, uint16_t& current_index) //& 是传reference 类似于传地址 函数内外改变同步
{
    mymemory.init(); //flash的打开 open the flash

    // 计算 FLASH 的用户区起始地址（靠近 flash 尾部）Calculates the start address of the FLASH user area 
    FLASH_BASE = mymemory.get_flash_start() + mymemory.get_flash_size() - (SECTOR_NUM * SECTOR_SIZE);

    uint16_t max_index = 0;
    uint32_t latest_addr = 0;

    PasswordEntry entry;

    // 遍历两个扇区 × 每页 Traverse two sectors x each page
    for (int sector = 0; sector < SECTOR_NUM; sector++) 
    {
        for (int page = 0; page < (SECTOR_SIZE / PAGE_SIZE); page++) 
        {
            uint32_t addr = FLASH_BASE + sector * SECTOR_SIZE + page * PAGE_SIZE;

            if (mymemory.read(&entry, addr, sizeof(PasswordEntry)) != 0)
                continue;
              
              // 计算实际校验和 Calculate the actual checksum
            uint16_t calc = calculate_checksum((char*)entry.passwords, MAX_PASSWORDS * PASSWORD_SIZE);

            // 如果校验通过且索引更大，则更新最新记录 If the check passes and the index is larger, the latest record is updated
            if (entry.checksum == calc && emtry.idx >= max_index) 
            {
                max_index = entry.idx;
                latest_addr = addr;
            }
        }
    }  

    // 如果没找到有效页 If no valid page is found
    if (latest_addr == 0) {
        mymemory.deinit();
        return false;
    }

    if (mymemory.read(&entry, latest_addr , sizeof(PasswordEntry)) != 0)  //根据read的定义=0为成功
    {
        mymemory.deinit();
        return false;
    }

    // 加载到 class Password 中 Load it into the class of Password
    pass.load((char*)entry.passwords); 

     /*void load(const char* raw) 
    {
        memcpy(passwords, raw, MAX_PASSWORDS * PASSWORD_SIZE)); 
    }
    need a   char passwords[MAX_PASSWORDS][PASSWORD_SIZE];*/

    current_index = entry.index;
    mymemory.deinit(); //flash的关闭 close the flash
    return true;
}


bool boot_save_passwords(const Password& pass, uint16_t current_index) 
{
    mymemory.init();

    // 下一页索引 Next page index
    uint16_t new_index = current_index + 1;
    // 计算页和扇区位置 Calculate page and sector locations
    uint32_t page_per_sector = SECTOR_SIZE / PAGE_SIZE;
    uint32_t new_page = new_index % page_per_sector;
    uint32_t new_sector = (new_index / page_per_sector) % SECTOR_NUM;

    // 如果是新扇区第一页，则需要先擦除整个扇区 If it is the first page of a new sector, you need to erase the entire sector first
    if (new_page == 0) 
    {
        mymemory.erase(FLASH_BASE + new_sector * SECTOR_SIZE, SECTOR_SIZE);
    }

    PasswordEntry entry;
    entry.index = new_index;

    pass.extract((char*)entry.passwords);  //写从class中提取密码的函数 May should write a function that extracts the password from the class
    /* void extract(char* out) const 
    {
        memcpy(out, passwords, MAX_PASSWORDS * PASSWORD_SIZE);
    }
    need a   char passwords[MAX_PASSWORDS][PASSWORD_SIZE];*/
    entry.checksum = calculate_checksum((char*)entry.passwords, MAX_PASSWORDS * PASSWORD_SIZE);

    uint32_t addr = FLASH_BASE + new_sector * SECTOR_SIZE + new_page * PAGE_SIZE;

    int status = mymemory.program(&entry, addr, sizeof(PasswordEntry)); //放进flash中 status == 0 表示成功 status == 0 indicates success
    mymemory.deinit();
    return status == 0;
}
*/

/*void Input(char key)
{
    if(key == '#') // clear
    {  
        memset(keypad_input, 0, bufferSize);
        input_index = 0;
        input_pos = 0;
        mydisplay.clear();
    }
    else if(key == '*') //enter
    {  
        if(input_pos == 8) 
        {
            //validate_password(input_buffer);
            pass.check_password(input_buffer);
             mydisplay.clear();
        }
    }
    else if(key >= '0' && key <= '9') 
    {  
        if(input_pos < 8) 
        {
            handle_input_display(key);
        }
    }
} */


void handle_input_display(char new_char) 
{
    if(new_char >= '0' && new_char <= '9') 
    {
        keypad_input[input_index] = new_char;
        input_index = (input_index + 1) % bufferSize;

        //input_buffer[input_pos++] = new_char;  //check

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












