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


int Password::check_password(char password_number_input[9])
{
    int check = 0;
    int i = 0;
    int n = 0;
    
    for( i = 0; i <= MAX_PASSWORDS; i++)
    {
        check = i;
        for (int n = 0; n <= MAX_LENGTH; n++)
        {
            if(password_number_input[n] != stored_passwords[i][n])
            {
                break;
            }
        if ( MAX_LENGTH == n )
        {
            return check;
        }
            
        }
    }
    return 99;

}

void Password::load_password(const char *raw) {

memcpy(stored_passwords, raw, MAX_LENGTH * MAX_PASSWORDS);

}

char* Password::save_password() {
static char raw[MAX_LENGTH * MAX_PASSWORDS];
memcpy(raw, stored_passwords, MAX_LENGTH * MAX_PASSWORDS);
return raw;
}