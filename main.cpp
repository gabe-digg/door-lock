/* mbed Microcontroller Library
 * Copyright (c) 2019 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
 */


#include "mbed.h"
#include "FlashIAP.h"
#include "SLCD.h"
#include "keypad.h"
#include <cstring>

#define FLASH_TOTAL_SIZE    0x00040000
#define PASSWORD_LENGTH 9
#define MAX_USERS 10
#define SECTOR_SIZE 1024
#define PAGE_SIZE 256

#define SECTOR1_ADDR 0x3F800
#define SECTOR2_ADDR 0x3FC00

#define DISPLAY_DIGITS 4
#define BUFFER_SIZE 32

struct FlashData 
{
    uint16_t index;
    char passwordlist[MAX_USERS][PASSWORD_LENGTH];
    uint16_t checksum;
};

FlashData current_structure;
char password_buffer[PASSWORD_LENGTH];
char password_change[PASSWORD_LENGTH];
char password_add[PASSWORD_LENGTH];
char display_buffer[BUFFER_SIZE] = {0};
int display_index = 0;
char first_key;

uint16_t calculate_checksum(const FlashData &data);
bool validate_checksum(const FlashData &data);
void factory_reset();
void init_flash();
void load_password();
void save_password();
int check_password(const char input[PASSWORD_LENGTH]);
bool delete_password(int serial);
void scroll_on_displayer(char new_char);
void input_password(char *buffer, char a);
char Output(const char* output_str);
int get_serial_input_from_keypad();

Keypad mykeypad(PTC8, PTA5, PTA4, PTA12, PTD3, PTA2, PTA1);
SLCD mydisplay;
FlashIAP myflash;

uint16_t calculate_checksum(const FlashData &data) 
{
    uint16_t sum = 0;
    for (int i = 0; i < MAX_USERS; i++) {
        for (int j = 0; j < PASSWORD_LENGTH; j++) {
            sum += data.passwordlist[i][j];
        }
    }
    sum += data.index;
    return sum;
}

bool validate_checksum(const FlashData &data) 
{
    return data.checksum == calculate_checksum(data);
}

void factory_reset() 
{
    myflash.erase(SECTOR1_ADDR, SECTOR_SIZE);
    myflash.erase(SECTOR2_ADDR, SECTOR_SIZE);

    memset(&current_structure, 0, sizeof(current_structure));
    current_structure.index = 1;
    strcpy(current_structure.passwordlist[0], "12345678");
    current_structure.checksum = calculate_checksum(current_structure);
    myflash.program(&current_structure, SECTOR1_ADDR, sizeof(FlashData));
}

void init_flash()
{
    myflash.init();
    FlashData temp;
    uint16_t max_index = 0;
    bool found_valid = false;

    for (int i = 0; i < 8; i++) 
    {
        uint32_t addr;
        if (i < 4) {
            addr = SECTOR1_ADDR + i * PAGE_SIZE;
        } 
        else {
            addr = SECTOR2_ADDR + (i - 4) * PAGE_SIZE;
        }
        myflash.read(&temp, addr, sizeof(FlashData));
        if (validate_checksum(temp) && temp.index > max_index) {
            max_index = temp.index;
            found_valid = true;
        }
    }

    if (!found_valid) {
        factory_reset();
    }
    myflash.deinit();
}

void load_password() 
{
    myflash.init();
    FlashData temp;
    uint16_t max_valid_index = 0;
    uint32_t max_valid_addr = 0;

    for (int i = 0; i < 8; i++) 
    {
        uint32_t addr;
        if (i < 4) {
            addr = SECTOR1_ADDR + i * PAGE_SIZE;
        }
        else {
            addr = SECTOR2_ADDR + (i - 4) * PAGE_SIZE;
        }

        myflash.read(&temp, addr, sizeof(FlashData));

        if (validate_checksum(temp) && temp.index > max_valid_index) {
            max_valid_index = temp.index;
            max_valid_addr = addr;
        }
    }

    myflash.read(&current_structure, max_valid_addr, sizeof(FlashData));
    myflash.deinit();
}

void save_password() 
{
    myflash.init();
    current_structure.index++;
    current_structure.checksum = calculate_checksum(current_structure);

    uint16_t new_page = (current_structure.index - 1) % 8;

    if (new_page == 0) {
        myflash.erase(SECTOR1_ADDR, SECTOR_SIZE);
    } 
    else if (new_page == 4) {
        myflash.erase(SECTOR2_ADDR, SECTOR_SIZE);
    }

    uint32_t addr;
    if (new_page < 4) {
        addr = SECTOR1_ADDR + new_page * PAGE_SIZE;
    } 
    else {
        addr = SECTOR2_ADDR + (new_page - 4) * PAGE_SIZE;
    }
    
    myflash.program(&current_structure, addr, sizeof(FlashData));
    myflash.deinit();
}

int check_password(const char input[PASSWORD_LENGTH]) 
{
    load_password();
    for (int i = 0; i < MAX_USERS; i++) 
    {
        bool match = true;
        for (int j = 0; j < PASSWORD_LENGTH; j++) {
            if (current_structure.passwordlist[i][j] != input[j]) {
                match = false;
                break;
            }
        }
        if (match) {
            return i + 1;
        }
    }
    return 99;

}

bool delete_password(int serial) 
{
    mydisplay.clear();
    mydisplay.Home();
    memset(display_buffer, 0, BUFFER_SIZE);

    for (int i = 0; i < 8; i++) 
    {
        char c = current_structure.passwordlist[serial][i];
        if (c >= '0' && c <= '9') {
            scroll_on_displayer(c);
        }
        ThisThread::sleep_for(300ms);
    }
    
    mydisplay.clear();
    mydisplay.Home();
    mydisplay.puts(" YE5");
    ThisThread::sleep_for(2s);
    mydisplay.clear();
    mydisplay.Home();
    mydisplay.puts(" OR ");
    ThisThread::sleep_for(1s);
    mydisplay.clear();
    mydisplay.Home();
    mydisplay.puts(" N0 ");
    ThisThread::sleep_for(2s);
    mydisplay.clear();
    mydisplay.Home();

    char confirm;
    do {
        confirm = mykeypad.ReadKey();
        ThisThread::sleep_for(10ms);
    } while (confirm != '#' && confirm != '*');

    if (confirm == '#') {
        memset(current_structure.passwordlist[serial], 0, PASSWORD_LENGTH);
        return true;
    }

    return false;
}

void add_password(int serial, char newpass[PASSWORD_LENGTH]) 
{
    
    memcpy(current_structure.passwordlist[serial], newpass, PASSWORD_LENGTH);
    
}


void scroll_on_displayer(char new_char) 
{
    if(new_char >= '0' && new_char <= '9') 
    {
        display_buffer[display_index] = new_char;
        display_index = (display_index + 1) % BUFFER_SIZE;

        mydisplay.clear();
        mydisplay.Home();
            
        int input_start = (display_index - DISPLAY_DIGITS + BUFFER_SIZE) % BUFFER_SIZE;
            
        for (int i = DISPLAY_DIGITS - 1; i >= 0; i--) 
        {
            char c = display_buffer[(input_start + i) % BUFFER_SIZE];
            mydisplay.putc(c);
        }
    }
}

void input_password(char *buffer, char a) 
{
    int index = 1;
    memset(buffer, 0, BUFFER_SIZE);
    memset(display_buffer, 0, BUFFER_SIZE);

    buffer[0] = a;
    scroll_on_displayer(a);

    while (true) 
    {
        char key;

        do {
            key = mykeypad.ReadKey();
            ThisThread::sleep_for(200ms);
        } while (!((key >= '0' && key <= '9') || key == '#' || key == '*'));

        if (key >= '0' && key <= '9') 
        {
            if (index < PASSWORD_LENGTH - 1) 
            {
                buffer[index++] = key;
                scroll_on_displayer(key);
            }
        } 
        else if (key == '#') 
        {
            buffer[index] = '\0';
            break;
        } 
        else if (key == '*') 
        {
            memset(buffer, 0, PASSWORD_LENGTH);
            index = 0;
            mydisplay.clear();
            mydisplay.Home();
            memset(display_buffer, 0, BUFFER_SIZE);
            display_index = 0;
        }
    }

}


char Output(const char* output_str) 
{
    const int print_len = strlen(output_str);
    const int scroll_times = 2;
    const int scroll_steps = print_len * scroll_times - DISPLAY_DIGITS + 1;
    char SLCD_output[DISPLAY_DIGITS + 1] = {' ', ' ', ' ', ' ', '\0'};
    char display_output[BUFFER_SIZE] = {0};
    char key = '\0';

    for (int i = 0; i < BUFFER_SIZE; i++) {
        display_output[i] = ' ';
    }

    for (int i = 0; i < print_len && i < BUFFER_SIZE - 1; ++i) {
        display_output[i] = output_str[i];
    }

    int output_start = 0;
    int total_steps = (print_len > DISPLAY_DIGITS) ? scroll_steps : (scroll_times * (DISPLAY_DIGITS + 1));

    for (int step = 0; step < total_steps; ++step) 
    {
        char key = mykeypad.ReadKey();
        if (key != '\0') return key;

        mydisplay.clear();
        mydisplay.Home();

        for (int s = 0; s < DISPLAY_DIGITS; ++s) 
        {
            int char_idx = (output_start + s) % print_len;
            SLCD_output[s] = display_output[char_idx];
        }

        SLCD_output[DISPLAY_DIGITS] = '\0';
        mydisplay.puts(SLCD_output);

        output_start = (output_start + 1) % print_len;
        ThisThread::sleep_for(300ms);
    }

    mydisplay.clear();
    mydisplay.Home();

    int start_idx = (print_len >= 4) ? (print_len - 4) : 0;
    int pad_spaces = DISPLAY_DIGITS - (print_len - start_idx);

    for (int i = 0; i < pad_spaces; ++i) {
        SLCD_output[i] = ' ';
    }
    for (int i = start_idx, j = pad_spaces; i < print_len && j < DISPLAY_DIGITS; ++i, ++j) {
        SLCD_output[j] = output_str[i];
    }
    SLCD_output[DISPLAY_DIGITS] = '\0';
    mydisplay.puts(SLCD_output);

    for (int i = 0; i < 1000000; ++i) {
        char key = mykeypad.ReadKey();
        if (key != '\0') return key;
        ThisThread::sleep_for(100ms);
    }

    mydisplay.clear();
    mydisplay.Home();
    return key;
}


int main() {
    init_flash();

    while (true) {
        first_key = Output(" PA55");
        mydisplay.clear();
        input_password(password_buffer, first_key);
        int user = check_password(password_buffer);

        if (user == 1) {  // Admin mode
            while (true) {
                first_key = Output(" 10PE 2CHA 3ADD 4DEL 5EX1 CH005E");
                mydisplay.clear();

                char opt;
                do {
                    opt = first_key;
                    ThisThread::sleep_for(200ms);
                } while (opt < '1' || opt > '5');

                if (opt == '1') {
                    mydisplay.puts("0PEN");
                    ThisThread::sleep_for(3s);
                    mydisplay.clear();
                    mydisplay.Home();
                    break;
                } 
                else if (opt == '2') 
                {
                    int sn = get_serial_input_from_keypad(); 
                    load_password();

                    if (delete_password(sn)) 
                    {
                        first_key = Output(" NEPA");
                        input_password(password_change, first_key);
                        add_password(sn, password_change);
                        memset(password_add, 0, PASSWORD_LENGTH);
                        save_password();
                        mydisplay.puts("D0NE");
                        ThisThread::sleep_for(3s);
                        mydisplay.clear();
                        mydisplay.Home();
                    }
                    else 
                    {
                        mydisplay.puts("FA1L");
                        ThisThread::sleep_for(3s);
                        mydisplay.clear();
                        mydisplay.Home();
                    }
                    
                    
                } 
                else if (opt == '3') 
                {
                    int sn = get_serial_input_from_keypad(); 
                    load_password();

                   
                    if(current_structure.passwordlist[sn][0] != 0)
                    {
                        mydisplay.clear();
                        mydisplay.Home(); 
                        mydisplay.puts("FA1L");
                        ThisThread::sleep_for(3s);
                        mydisplay.clear();
                        mydisplay.Home();
                    }
                    else
                    {
                        first_key = Output(" NEPA");
                        input_password(password_add, first_key);
                        add_password(sn, password_add);
                        memset(password_add, 0, PASSWORD_LENGTH);
                        save_password();
                        mydisplay.clear();
                        mydisplay.Home(); 
                        mydisplay.puts(" ADD");
                        ThisThread::sleep_for(3s);
                        mydisplay.clear();
                        mydisplay.Home();

                    }
                } 
                else if (opt == '4')
                 { 
                    int sn = get_serial_input_from_keypad(); 
                    load_password();

                    if (delete_password(sn)) 
                    {
                        save_password();
                        mydisplay.puts("D0NE");
                        ThisThread::sleep_for(3s);
                        mydisplay.clear();
                        mydisplay.Home();;
                    } 
                    else 
                    {
                        mydisplay.puts("FA1L");
                        ThisThread::sleep_for(3s);
                        mydisplay.clear();
                        mydisplay.Home();
                    }
                } 
                else if (opt == '5') {
                    break; 
                }
            }
        } 
        else if (user >= 2 && user <= 10) {
            mydisplay.puts("0PEN");
            ThisThread::sleep_for(3s);
            mydisplay.clear();
            mydisplay.Home();
        } 
        else {
            mydisplay.puts("FA1L");
            ThisThread::sleep_for(3s);
            mydisplay.clear();
            mydisplay.Home();
        }
    }
}

int get_serial_input_from_keypad()
{
    char buffer[2] = {0};
    int index = 0;

    mydisplay.clear();
    mydisplay.Home();


    for (int a = 0; a < 2; a++)
    {
        buffer[a] = 0;
    }
    char key = Output(" U5ER");

    if (key >= '0' && key <= '9') {
        buffer[index++] = key;
        mydisplay.clear();
        mydisplay.Home();
        for (int i = 0; i < index; i++) {
            mydisplay.putc(buffer[i]);
        }
    } else if (key == '#') {
        return (atoi(buffer)-1);
    }
    return (atoi(buffer)-1);
}