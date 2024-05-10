#include <stdint.h>
#include <stddef.h>
#include "header/stdlib/string.h"

void* memset(void *s, int c, size_t n) {
    uint8_t *buf = (uint8_t*) s;
    for (size_t i = 0; i < n; i++)
        buf[i] = (uint8_t) c;
    return s;
}

void* memcpy(void* restrict dest, const void* restrict src, size_t n) {
    uint8_t *dstbuf       = (uint8_t*) dest;
    const uint8_t *srcbuf = (const uint8_t*) src;
    for (size_t i = 0; i < n; i++)
        dstbuf[i] = srcbuf[i];
    return dstbuf;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    const uint8_t *buf1 = (const uint8_t*) s1;
    const uint8_t *buf2 = (const uint8_t*) s2;
    for (size_t i = 0; i < n; i++) {
        if (buf1[i] < buf2[i])
            return -1;
        else if (buf1[i] > buf2[i])
            return 1;
    }

    return 0;
}

void *memmove(void *dest, const void *src, size_t n) {
    uint8_t *dstbuf       = (uint8_t*) dest;
    const uint8_t *srcbuf = (const uint8_t*) src;
    if (dstbuf < srcbuf) {
        for (size_t i = 0; i < n; i++)
            dstbuf[i]   = srcbuf[i];
    } else {
        for (size_t i = n; i != 0; i--)
            dstbuf[i-1] = srcbuf[i-1];
    }

    return dest;
}

char *strcat(char *dest, const char *src){
    size_t i,j;
    for (i = 0; dest[i] != '\0'; i++)
        ;
    for (j = 0; src[j] != '\0'; j++)
        dest[i+j] = src[j];
    dest[i+j] = '\0';
    return dest;
}

void *strcpy(char *dest, const char *src){
    // assumsi dest sudah memiliki memory setidaknya n+1
    size_t i=0, n = strlen(src);
    for (; i < n; i++)
        dest[i] = src[i];
    dest[i+1] = '\0';
    return dest;
}

size_t strlen(const char* str) {
    size_t i;
    while (str[i] != '\0')
        i++;
    
    return i;
}

int strcmp(const char *str1, const char *str2) {
    if (strlen(str1) != strlen(str2)) {
        return 0;
    }

    for (size_t i=0; i<strlen(str1); i++) {
        if (str1[i] != str2[i]) {
            return 0;
        }
    }
    
    return 1;
}
