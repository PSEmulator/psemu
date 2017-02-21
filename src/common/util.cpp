#include <chrono>
#include <random>
#include <string>
#include <thread>
#include <assert.h>
#include "bitstream.h"
#include "util.h"

uint32_t randomUnsignedInt() {
    std::random_device randomDevice;
    std::mt19937_64 generator(randomDevice());
    std::uniform_int_distribution<uint32_t> distribution;

    return distribution(generator);
}

uint8_t randomUnsignedChar() {
    std::random_device randomDevice;
    std::mt19937_64 generator(randomDevice());
    std::uniform_int_distribution<uint32_t> distribution(0, UCHAR_MAX);

    return distribution(generator);
}

std::size_t getTimeSeconds() {
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    std::chrono::system_clock::duration tp = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::seconds>(tp).count();
}

std::size_t getTimeMilliseconds() {
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    std::chrono::system_clock::duration tp = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(tp).count();
}

std::size_t getTimeNanoseconds() {
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    std::chrono::system_clock::duration tp = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(tp).count();
}

void utilSleep(size_t ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

int hexCharToInt(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    } else if (c >= 'A' && c <= 'F') {
        return 10 + c - 'A';
    } else if (c >= 'a' && c <= 'f') {
        return 10 + c - 'A';
    }

    return -1;
}

std::vector<uint8_t> hexToBytes(std::string hexStr) {
    std::vector<uint8_t> bytes;

    uint8_t curValue = 0;
    bool firstDigitFilled = false;
    for (auto c : hexStr) {
        int charValue = hexCharToInt(c);

        if (charValue != -1) {
            if (!firstDigitFilled) {
                curValue = 16 * charValue;

                firstDigitFilled = true;
            } else {
                bytes.push_back(curValue + charValue);

                firstDigitFilled = false;
                curValue = 0;
            }
        }
    }

    return bytes;
}

/**
    Compare two float values for relative equality or the direction of inequality.
	@param a the first float
	@param b the second float
	@param error the margin of difference between parameter a and parameter b
	@throw invalid_argument if error is greater than 4 * 1024 * 1024
	@return a value representing the equality or inequality of the two input numbers
*/
const int compareFloatValues(const float a, const float b, const size_t error) {
	/*
	A simple decimal comparison might involve subtraction, fabs, and the result being less than a smaller decimal.
	The method used, however, converts the IEEE float values into integers that can be lexicographically ordered.
	Error becomes a count of "the maximum number of lexico-floats allowed between lexico-a and lexico-b."
	To avoid encountering a NaN value, error can not risk being too big.
	*/
	if(error >= 4194304) {
		throw std::invalid_argument("float comparison - error margin too big");
	}

	int aInt = *(int*)&a;
	if(aInt < 0) {
		aInt = 0x80000000 - aInt; //negative zero -> positive zero?
	}
	int bInt = *(int*)&b;
	if(bInt < 0) {
		bInt = 0x80000000 - bInt; //negative zero -> positive zero?
	}
	//borrows from Java's Comparator compare(T o1, T o2) model for return values.
	if(std::abs(aInt - bInt) <= error) {
		return 0;
	}
	else if(aInt + error < bInt) {
		return -1;
	}
	else {
		return 1;
	}
}
