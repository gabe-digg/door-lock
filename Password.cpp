#include "Password.h"
#include <cstring>

// Constructor - Default passwords
Password::Password() : password_count(0) {
    char trial[9] = "12345678";
    add_password (trial); // Add a default password
}




// Add a new password (if space allows)
bool Password::add_password( char new_password[MAX_LENGTH + 1] ) {
    if (password_count >= MAX_PASSWORDS || strlen(new_password) > MAX_LENGTH) {
        return false;
    }
    strncpy(stored_passwords[password_count], new_password, MAX_LENGTH);
    stored_passwords[password_count][MAX_LENGTH] = '\0';
    password_count++;
    return true;
}


bool Password::delete_password(const char* password_to_delete) {
    for (int i = 0; i < password_count; i++) {
        if (strncmp(stored_passwords[i], password_to_delete, MAX_LENGTH) == 0) {
            // Shift remaining passwords down
            for (int j = i; j < password_count - 1; j++) {
                strncpy(stored_passwords[j], stored_passwords[j + 1], MAX_LENGTH);
            }
            password_count--; // Reduce the count
            return true; // Successfully deleted
        }
    }
    return false; // Password not found
}

// Check if input matches any stored password
int Password::check_password( char input[MAX_LENGTH + 1]) {
    for (int i = 0 ; i < password_count; i++) {
        if (strncmp(stored_passwords[i], input, MAX_LENGTH) == 0) {
            return i; // Match found
        }
    }
    return 99;
}

