#pragma once
#include <cstddef>

extern "C" {

char* strcpy(char* dest, const char* src);
char* strncpy(char* dest, const char* src, size_t count);
char* strcat(char* dest, const char* src);
char* strncat(char* dest, const char* src, size_t count);

size_t strlen(const char* x);
int strcmp(const char* a, const char* b);
int strncmp(const char* a, const char* b, size_t count);
}

const char* strchr(const char* str, int ch);
char* strchr(char* str, int ch);
//char* variant through macro ?

const char* strrchr(const char* str, int ch);
char* strrchr(char* str, int ch);

size_t strspn(const char* dest, const char* src);
size_t strcspn(const char* dest, const char* src);

const char* strpbrk(const char* dest, const char* src);

const char* strstr(const char* str, const char* substr);
/*
Not yet implemented : strstr, strtok, strerror
Not implemented : strcoll
*/
extern "C" {
void* memchr(void* s, int c, size_t n);
int   memcmp(const void* a, const void* b, size_t n);
void  memset(void* a, int ch, size_t n);
void  memcpy(void* dest, const void* src, size_t n);
void  memmove(void* dest, const void* src, size_t n); 
}
