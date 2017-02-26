#pragma once

uint32_t randomUnsignedInt();
uint8_t randomUnsignedChar();
std::size_t getTimeSeconds();
std::size_t getTimeMilliseconds();
std::size_t getTimeNanoseconds();

void utilSleep(size_t ms);

std::vector<uint8_t> hexToBytes(std::string hexStr);

int compareDecimals(double a, double b, double epsilon);
