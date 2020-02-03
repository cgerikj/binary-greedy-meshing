#ifndef CONSTANTS_H
#define CONSTANTS_H

// CS = chunk size (max 62)
static constexpr int CS = 62;
static constexpr int Y_CHUNKS = 16;

// Padded chunk size
static constexpr int CS_P = CS + 2;
static constexpr int CS_P2 = CS_P * CS_P;
static constexpr int CS_P3 = CS_P * CS_P * CS_P;

#endif