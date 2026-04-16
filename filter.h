#ifndef FILTER_H
#define FILTER_H

#include <stdint.h>

#define FILTER_SIZE 18

typedef struct {
    int32_t samples[FILTER_SIZE];
    int32_t sort_buffer[FILTER_SIZE];  // Siralama için ayri buffer
    uint8_t index;
    uint8_t count;  // Kaç deger eklendigini takip et
} MedianFilter_t;

int32_t ApplyMedianFilter18(MedianFilter_t *f, int32_t new_val);

#endif