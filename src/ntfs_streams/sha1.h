#pragma once

#include <stdint.h>
#include <array>


std::array<uint8_t, 20> sha1_digest(const void* message, unsigned int length);