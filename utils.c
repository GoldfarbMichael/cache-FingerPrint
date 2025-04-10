#include "utils.h"
#include <stdio.h>
#include <string.h>

void parse_site_name(const char* url, char* site_name, size_t size) {
    const char* www_ptr = strstr(url, "www.");
    if (!www_ptr) {
        fprintf(stderr, "Invalid URL: 'www.' not found\n");
        return;
    }

    www_ptr += 4; // Move past "www."
    const char* dot_ptr = strchr(www_ptr, '.');
    if (!dot_ptr) {
        fprintf(stderr, "Invalid URL: '.' not found after 'www.'\n");
        return;
    }

    size_t length = dot_ptr - www_ptr;
    if (length >= size) {
        fprintf(stderr, "Site name buffer too small\n");
        return;
    }

    strncpy(site_name, www_ptr, length);
    site_name[length] = '\0'; // Null-terminate the string
}
