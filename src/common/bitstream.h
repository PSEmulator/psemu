#pragma once

#include <iostream>
#include <vector>

/**
 * A stream serializer that reads/writes to an external buffer.
 *
 * Serialization of bit strings is big endian, but everything else is little endian.
 *
 * The stream bit pos IS allowed to be at buf.size() * 8 (technically past the end of the buffer range),
 * but should never be beyond that, and should not be able to actually read/write at that point without
 * additional buffer allocation.
 */
class BitStream {
public:
    BitStream(std::vector<uint8_t>& exisitingBuf) :
        buf(exisitingBuf),
        streamBitPos(0),
        error(false) {

    }

    /**
     * @return The number of bytes required to hold the number of bits
     */
    size_t bitsToReqBytes(size_t bits) const {
        return (bits + 7) / 8;
    }

    /**
     * @return The total number of bits in the buffer
     */
    size_t getSizeBits() const {
        return buf.size() * 8;
    }

    /**
     * @return The number of unread bits after the stream pos
     */
    size_t getRemainingBits() const {
        size_t sizeBits = getSizeBits();
        if (streamBitPos >= sizeBits) {
            return 0;
        } else {
            return sizeBits - streamBitPos;
        }
    }

    /**
     * @return The number of COMPLETELY unread bytes after the stream pos
     */
    size_t getRemainingBytes() const {
        if (bitsToReqBytes(streamBitPos) >= buf.size()) {
            return 0;
        } else {
            return buf.size() - bitsToReqBytes(streamBitPos);
        }
    }

    /**
     * @return A pointer to the byte holding the stream pos
     */
    uint8_t* getHeadBytePtr() const {
        return buf.data() + (streamBitPos / 8);
    }

    /**
     * @return An iterator to the byte holding the stream pos
     */
    std::vector<uint8_t>::iterator getHeadIterator() const {
        return buf.begin() + (streamBitPos / 8);
    }

    /**
     * @return A pointer to the byte holding the given bit pos
     */
    uint8_t* getPosBytePtr(size_t bitPos) const {
        return buf.data() + (bitPos / 8);
    }

    /**
     * @return The current stream pos
     */
    size_t getPos() const {
        return streamBitPos;
    }

    /**
     * Sets the current stream pos.
     */
    void setPos(size_t pos) {
        if (pos > buf.size() * 8) {
            error = true;
            return;
        }

        streamBitPos = pos;
    }

    /**
     * Moves the stream pos by some delta.
     */
    void deltaPos(int32_t delta) {
        // TODO: Signedness is funky here. Figure out a better way.
        int32_t newPos = (int32_t)streamBitPos + delta;

        if (newPos < 0 || newPos > buf.size() * 8) {
            error = true;
            return;
        }

        streamBitPos = newPos;
    }

    /**
     * Aligns the stream pos to the next highest byte boundary.
     */
    void alignPos() {
        size_t bitsIn = (streamBitPos % 8);
        if (bitsIn != 0) {
            deltaPos(8 - bitsIn);
        }
    }

    /**
     * @return True if the stream has encountered a serialization error.
     */
    bool getError() {
        return error;
    }

    /**
     * Reads a number of bytes.
     * The stream pos must be aligned on a byte boundary or an error will occur.
     */
    void readBytes(uint8_t* outBuf, size_t numBytes, bool peek = false) {
        if (numBytes == 0) {
            return;
        }

        if (streamBitPos % 8 != 0) {
            std::cout << "Not aligned read bytes!" << std::endl;
            error = true;
            return;
        }

        size_t remainingBytes = getRemainingBytes();
        if (remainingBytes < numBytes) {
            std::cout << "Requested too big read bytes!" << std::endl;
            error = true;
            return;
        }

        memcpy(outBuf, getHeadBytePtr(), numBytes);

        if (!peek) {
            streamBitPos += numBytes * 8;
        }
    }

    /**
     * @return The value of the next bit in the stream
     */
    bool readBit(bool peek = false) {
        size_t remainingBits = getRemainingBits();
        if (remainingBits < 1) {
            std::cout << "Requested too big read bits!" << std::endl;
            error = true;
            return false;
        }

        bool result = (*getHeadBytePtr() & (0x80 >> (streamBitPos & 0x7))) != 0;

        streamBitPos++;

        return result;
    }

    /**
     * Reads a number of bits.
     */
    void readBits(uint8_t* outBuf, size_t numBits, bool peek = false) {
        if (numBits == 0) {
            return;
        }

        size_t remainingBits = getRemainingBits();
        if (remainingBits < numBits) {
            std::cout << "Requested too big read bits!" << std::endl;
            error = true;
            return;
        }

        // TODO: Can optimize this by going byte-by-byte rather than bit-by-bit with some fancy shifting/masking
        for (size_t dataBitPos = 0; dataBitPos < numBits; ++dataBitPos) {
            size_t invBitPos = streamBitPos + numBits - 1 - dataBitPos;
            if ((*getPosBytePtr(invBitPos) & (0x80 >> (invBitPos & 0x7))) != 0) {
                *(outBuf + ((numBits - 1 - dataBitPos) / 8)) |= (1 << (dataBitPos & 0x7));
            } else {
                *(outBuf + ((numBits - 1 - dataBitPos) / 8)) &= ~(1 << (dataBitPos & 0x7));
            }
        }

        streamBitPos += numBits;
    }

    /**
     * Writes a number of bytes.
     * The stream pos must be aligned on a byte boundary or an error will occur.
     */
    void writeBytes(const uint8_t* data, size_t numBytes) {
        if (numBytes == 0) {
            return;
        }

        if (streamBitPos % 8 != 0) {
            std::cout << "Not aligned write bytes!" << std::endl;
            error = true;
            return;
        }

        size_t remainingBytes = getRemainingBytes();
        if (remainingBytes < numBytes) {
            buf.resize(buf.size() + numBytes - remainingBytes);
        }

        memcpy(getHeadBytePtr(), data, numBytes);

        streamBitPos += numBytes * 8;
    }

    /**
     * Writes a boolean value as a single bit.
     */
    void writeBit(bool value) {
        size_t remainingBits = getRemainingBits();
        if (remainingBits < 1) {
            buf.resize(buf.size() + 1);
        }

        if (value) {
            *getHeadBytePtr() |= (0x80 >> (streamBitPos & 0x7));
        } else {
            *getHeadBytePtr() &= ~(0x80 >> (streamBitPos & 0x7));
        }

        streamBitPos++;
    }

    /**
     * Reads a number of bits.
     */
    void writeBits(const uint8_t* data, size_t numBits) {
        if (numBits == 0) {
            return;
        }

        size_t remainingBits = getRemainingBits();
        if (remainingBits < numBits) {
            buf.resize(bitsToReqBytes(buf.size() * 8 + numBits - remainingBits));
        }

        // TODO: Can optimize this by going byte-by-byte rather than bit-by-bit with some fancy shifting/masking
        for (size_t dataBitPos = 0; dataBitPos < numBits; ++dataBitPos) {
            size_t invBitPos = streamBitPos + numBits - 1 - dataBitPos;
            if ((*(data + ((numBits - 1 - dataBitPos) / 8)) & (1 << (dataBitPos & 0x7))) != 0) {
                *getPosBytePtr(invBitPos) |= (0x80 >> (invBitPos & 0x7));
            } else {
                *getPosBytePtr(invBitPos) &= ~(0x80 >> (invBitPos & 0x7));
            }
        }

        streamBitPos += numBits;
    }

    /**
     * Templated read functions.
     */
    template<typename T, size_t arraySize>
    void read(std::array<T, arraySize>& outBuf, bool peek = false) {
        static_assert(!std::is_pointer<T>::value, "Object type must not be a pointer.");

        readBytes(outBuf.data(), sizeof(T) * outBuf.size(), peek);
        if (error) {
            std::cout << "Unaligned? Trying read bits instead..." << std::endl;
            error = false;
            readBits(outBuf.data(), 8 * sizeof(T) * outBuf.size(), peek);
        }
    }

    template<typename T>
    void read(T& object, bool peek = false) {
        static_assert(!std::is_pointer<T>::value, "Object type must not be a pointer.");

        readBytes((uint8_t*)&object, sizeof(T), peek);
        if (error) {
            std::cout << "Unaligned? Trying read bits instead..." << std::endl;
            error = false;
            readBits((uint8_t*)&object, 8 * sizeof(T), peek);
        }
    }

	/**
	 * Read the length of the following string from the bitstream as a big-endian number.
	 *
	 * PlanetSide packets mainly use length-prefaced strings, where the length is either one byte or two bytes.
	 * If the length is less than 128, the first bit of the field is "on" and the length is the remaining seven bits.
	 * If the length is 128 or greater, the first bit of the field is "off" and the length is the remaining fifteen bits.
	 * In other words: 0 <= n < 128 is 1 byte, from 0x80 to 0xFF;
	 * and, 128 <= n < 32767 is 2 bytes, from 0x0080 to 0x7FFF.
	 *
	 * @param bitStream the stream, with the current position at the length field
	 * @return the length
	 */
	uint16_t readStringLength() {
		uint16_t strLen = 0;
		if (readBit()) {
			readBits((uint8_t*)&strLen, 7);
		}
		else {
			readBits((uint8_t*)&strLen, 15);
		}
		return strLen;
	}

    void read(std::string& str) {
		uint16_t strLen = readStringLength();

        alignPos();

        str.resize(strLen);
        readBytes((uint8_t*)str.data(), strLen);
    }

    void read(std::wstring& str) {
        uint16_t strLen = readStringLength();

        alignPos();

        str.resize(strLen);
        readBytes((uint8_t*)str.data(), strLen * 2);
    }

    /**
     * Templated write functions.
     */
    template<typename T, size_t arraySize>
    void write(const std::array<T, arraySize>& data) {
        static_assert(!std::is_pointer<T>::value, "Object type must not be a pointer.");

        writeBytes(data.data(), sizeof(T) * data.size());
        if (error) {
            std::cout << "Unaligned? Trying write bits instead..." << std::endl;
            error = false;
            writeBits(data.data(), 8 * sizeof(T) * data.size());
        }
    }

    template<typename T>
    void write(const std::vector<T>& data) {
        static_assert(!std::is_pointer<T>::value, "Object type must not be a pointer.");

        writeBytes(data.data(), sizeof(T) * data.size());
        if (error) {
            std::cout << "Unaligned? Trying write bits instead..." << std::endl;
            error = false;
            writeBits(data.data(), 8 * sizeof(T) * data.size());
        }
    }

    template<typename T>
    void write(const T& object) {
        static_assert(!std::is_pointer<T>::value, "Object type must not be a pointer.");

        writeBytes((const uint8_t*)&object, sizeof(T));
        if (error) {
            std::cout << "Unaligned? Trying write bits instead..." << std::endl;
            error = false;
            writeBits((const uint8_t*)&object, 8 * sizeof(T));
        }
    }

	/**
	 * Write the length of the following string into the bitstream as a big-endian number.
	 *
	 * Comments in readStringLength are still valid for this function.
	 *
	 * @param strLen the string's length
	 */
	void writeStringLength(size_t strLen) {
		bool isShortString = (strLen < 128);
		writeBit(isShortString);
		if (isShortString) {
			writeBits((uint8_t*)&strLen, 7);
		}
		else {
			writeBits((uint8_t*)&strLen, 15);
		}
	}

    void write(const std::string& str) {
        size_t strLen = str.length();
		writeStringLength(strLen);

        alignPos();

        writeBytes((uint8_t*)str.data(), strLen);
    }

    void write(const std::wstring& str) {
        size_t strLen = str.length();
		writeStringLength(strLen);

        alignPos();

        writeBytes((uint8_t*)str.data(), strLen * 2);
    }

    std::vector<uint8_t>& buf;

private:
    size_t streamBitPos;
    bool error;
};
