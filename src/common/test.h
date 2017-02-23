#pragma once

#define assertBuffersEqual(actual, expected, ...) do {\
    if (actual != expected) {\
        std::cout << "Assert failed! File \"" << __FILE__ << "\" line " << __LINE__ << std::endl;\
        std::cout << std::hex << std::uppercase << "actual (" << #actual << ") vs expected (" << #expected << "): " << std::endl << "{" << strHex(actual) << "}" << std::endl << "{" << strHex(expected) << "}" << std::dec << std::nouppercase << std::endl << std::endl;\
        printAssertMessage(##__VA_ARGS__);\
    }\
} while (0)

#define assertEqual(actual, expected, ...) do {\
    if (actual != expected) {\
        std::cout << "Assert failed! File \"" << __FILE__ << "\" line " << __LINE__ << std::endl;\
        std::cout << std::hex << std::uppercase << "actual (" << #actual << ") vs expected (" << #expected << "): " << std::endl << "{" << actual << "}" << std::endl << "{" << expected << "}" << std::dec << std::nouppercase << std::endl << std::endl;\
        printAssertMessage(##__VA_ARGS__);\
    }\
} while (0)

void printAssertMessage();
void printAssertMessage(const char* message);
