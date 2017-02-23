#pragma once

uint32_t randomUnsignedInt();
uint8_t randomUnsignedChar();
std::size_t getTimeSeconds();
std::size_t getTimeMilliseconds();
std::size_t getTimeNanoseconds();

void utilSleep(size_t ms);

std::vector<uint8_t> hexToBytes(std::string hexStr);

/**
Compare two float values for relative equality or the direction of inequality.
@param a the first float
@param b the second float
@param epsilon the margin of difference between parameter a and parameter b
@throw invalid_argument if epsilon is greater than 4 * 1024 * 1024
@return a negative integer, zero, or a positive integer as the first argument is <, ==, or > the second
*/
const int compareFloats(const float a, const float b, const size_t error);
