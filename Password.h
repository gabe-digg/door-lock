#pragma once
#include "mbed.h"

class Password {
private:
    static constexpr int MAX_LENGTH = 8;
    static constexpr int MAX_PASSWORDS = 9; // Support up to 5 passwords
    char stored_passwords[MAX_PASSWORDS][MAX_LENGTH + 1];
    int password_count;

public:
    // Constructor
    Password();

    // Validate input against stored passwords
    int check_password( char input[MAX_LENGTH + 1]);

    // Add a new password (if space allows)
    bool add_password( char new_password[MAX_LENGTH + 1]);

    // we take the entire password we want to delete as an input
    bool delete_password (const char* password_to_delete);

    

};