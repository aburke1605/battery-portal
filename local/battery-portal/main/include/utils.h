#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

void initialise_nvs();

void initialise_spiffs();

void convert_uint_to_n_bytes(unsigned int input, uint8_t *output, size_t n_bytes, bool little_endian);

#endif // UTILS_H
