#pragma once

#ifndef RESOURCE_T_DEFINED
#define RESOURCE_T_DEFINED

#include <stdint.h>

struct resource_t {
  resource_t(const char* name, const uint8_t* data, uint32_t size) : name(name), data(data), size(size) {}
  const char* name; const uint8_t* data; const uint32_t size;
};
#endif

extern const uint8_t ROBOTO_REGULAR[145348];
extern const int ROBOTO_REGULAR_length;

