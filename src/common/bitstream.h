#pragma once

#include <algorithm>
#include <iostream>
#include <vector>
#include <math.h>

#define BITS_TO_BYTES(bits) (((bits) + 7) / 8)

/**
 * A stream serializer that reads/writes to an external buffer.
 *
 * Serialization of bits starts at the MSB toward LSB, and all data is serialized in memory byte-order.
 *
 * The stream bit pos IS allowed to be at buf.size() * 8 (technically past the end of the buffer range),
 * but should never be beyond that, and should not be able to actually read/write at that point without
 * additional buffer allocation.
 */
class BitStream {
public:
    enum class Error {
        NONE,
        INVALID_STREAM_POS,
        READ_TOO_MUCH
    };

    BitStream(std::vector<uint8_t>& exisitingBuf) :
        buf(exisitingBuf),
        streamBitPos(0),
        lastError(Error::NONE) {

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
        size_t usedBytes = BITS_TO_BYTES(streamBitPos);
        if (usedBytes >= buf.size()) {
            return 0;
        } else {
            return buf.size() - usedBytes;
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
            lastError = Error::INVALID_STREAM_POS;
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
            lastError = Error::INVALID_STREAM_POS;
            return;
        }

        streamBitPos = newPos;
    }

    /**
     * Aligns the stream pos to the next highest byte boundary if necessary.
     */
    void alignPos() {
        size_t bitsIn = (streamBitPos % 8);
        if (bitsIn != 0) {
            deltaPos(8 - bitsIn);
        }
    }

    /**
     * @return The error type if the stream has encountered a serialization error.
     */
    Error getLastError() {
        return lastError;
    }

    /**
     * Reads a number of bytes.
     */
    void readBytes(uint8_t* outBuf, size_t numBytes, bool peek = false) {
        if (numBytes == 0) {
            return;
        }

        // If the stream position is not aligned on a byte boundary, need to do bit reading
        if ((streamBitPos & 0x7) != 0) {
            readBits(outBuf, numBytes * 8, peek);
            return;
        }

        size_t remainingBytes = getRemainingBytes();
        if (remainingBytes < numBytes) {
            lastError = Error::READ_TOO_MUCH;
            return;
        }

        memcpy(outBuf, getHeadBytePtr(), numBytes);

        if (!peek) {
            streamBitPos += numBytes * 8;
        }
    }

    /**
     * @return The boolean value of the next bit in the stream
     */
    bool readBit(bool peek = false) {
        size_t remainingBits = getRemainingBits();
        if (remainingBits < 1) {
            lastError = Error::READ_TOO_MUCH;
            return false;
        }

        bool result = (*getHeadBytePtr() & (0x80 >> (streamBitPos & 0x7))) != 0;

        streamBitPos++;

        return result;
    }

    /**
     * Reads a number of bits.
     * NOTE: WILL clobber any existing data in the bytes that get written to
     */
    void readBits(uint8_t* outBuf, size_t numBits, bool peek = false) {
        if (numBits == 0) {
            return;
        }

        // If the stream position is aligned on a byte boundary and we are reading a quantity of bits divisble by 8, we can use faster byte reading
        if ((streamBitPos & 0x7) == 0 && (numBits & 0x7) == 0) {
            readBytes(outBuf, numBits / 8, peek);
            return;
        }

        size_t remainingBits = getRemainingBits();
        if (remainingBits < numBits) {
            lastError = Error::READ_TOO_MUCH;
            return;
        }
        
        size_t prevStreamBitPos = streamBitPos;

        unsigned char* srcPtr = buf.data();

        size_t bitsToRead = numBits;

        while (true) {
            size_t byteOffset = streamBitPos / 8;
            size_t bitOffset = (streamBitPos & 0x7);
            size_t bitsLeft = 8 - bitOffset;

            if (bitsLeft >= bitsToRead) {
                // We have enough bits remaining in the current source byte to finish the read, so read them all into the current destination byte
                // Shift the remaining bits right to be flush with the start of the current destination byte, and mask out the bits to the left
                size_t bitGap = (bitsLeft - bitsToRead);
                *outBuf = ((srcPtr[byteOffset] >> bitGap) & ((1 << bitsToRead) - 1));
                streamBitPos += bitsToRead;
                break;
            } else {
                // We don't have enough bits remaining in the current byte to finish the read, so read as much as we need into the current destination byte
                // Shift the current source byte left to reserve a number of bits on the right to read from the next byte, and mask out the bits to the left
                size_t bitsToWriteToSrc = std::min(bitsToRead, (size_t)8);
                size_t bitsToReserve = bitsToWriteToSrc - bitsLeft;

                *outBuf = ((srcPtr[byteOffset] & ((1 << bitsLeft) - 1)) << bitsToReserve);

                // Read the rest of the bits we reserved room for in the destination byte from the next source byte
                *outBuf |= (srcPtr[byteOffset + 1] >> (8 - bitsToReserve));

                streamBitPos += bitsToWriteToSrc;
                bitsToRead -= bitsToWriteToSrc;

                if (bitsToRead == 0) {
                    break;
                }

                outBuf++;
            }
        }

        if (peek) {
            streamBitPos = prevStreamBitPos;
        }
    }

    /**
     * Writes a number of bytes.
     */
    void writeBytes(const uint8_t* data, size_t numBytes) {
        if (numBytes == 0) {
            return;
        }

        // If the stream position is not aligned on a byte boundary, need to do bit writing
        if ((streamBitPos & 0x7) != 0) {
            writeBits(data, numBytes * 8);
            return;
        }

        // Reserve space for the number of bytes we're going to write
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
     * Writes a number of bits.
     */
    void writeBits(const uint8_t* data, size_t numBits) {
        if (numBits == 0) {
            return;
        }

        // If the stream position is aligned on a byte boundary and we are writing a quantity of bits divisble by 8, we can use faster byte writing
        if ((streamBitPos & 0x7) == 0 && (numBits & 0x7) == 0) {
            writeBytes(data, numBits / 8);
            return;
        }

        // Reserve space for the number of bits we're going to write
        size_t remainingBits = getRemainingBits();
        if (remainingBits < numBits) {
            buf.resize(BITS_TO_BYTES(buf.size() * 8 + numBits - remainingBits));
        }

        unsigned char* dstPtr = buf.data();

        size_t bitsToWrite = numBits;

        while (true) {
            size_t byteOffset = streamBitPos / 8;
            size_t bitOffset = (streamBitPos & 0x7);
            size_t bitsLeft = 8 - bitOffset;

            if (bitsLeft >= bitsToWrite) {
                // We have enough room in the current byte to fit all of the rest of the bits, so write them all from the current source byte
                // Shift the remaining bits left to close the gap and be flush with the end of the stream
                size_t bitGap = (bitsLeft - bitsToWrite);
                dstPtr[byteOffset] |= (*data << bitGap);
                streamBitPos += bitsToWrite;
                break;
            } else {
                // We don't have enough room for all of the remaining bits, so write as much as we need to from the current source byte
                // Shift the current byte right to un-overlap the bits and be flush with the end of the stream
                size_t bitsToWriteFromSrc = std::min(bitsToWrite, (size_t)8);
                size_t bitsOverlapped = bitsToWriteFromSrc - bitsLeft;

                dstPtr[byteOffset] |= (*data >> bitsOverlapped);

                // Now write the rest of the bits remaining in the current source byte to the next destination byte
                dstPtr[byteOffset + 1] |= (*data << (8 - bitsOverlapped));

                streamBitPos += bitsToWriteFromSrc;
                bitsToWrite -= bitsToWriteFromSrc;

                if (bitsToWrite == 0) {
                    break;
                }

                data++;
            }
        }
    }

    /**
     * Templated read functions.
     */
    template<typename T, size_t arraySize>
    void read(std::array<T, arraySize>& outBuf, bool peek = false) {
        static_assert(!std::is_pointer<T>::value, "Object type must not be a pointer.");

        readBytes(outBuf.data(), sizeof(T) * outBuf.size(), peek);
    }

    template<typename T>
    void read(T& object, bool peek = false) {
        static_assert(!std::is_pointer<T>::value, "Object type must not be a pointer.");

        readBytes((uint8_t*)&object, sizeof(T), peek);
    }

    uint16_t readStringLength() {
        uint16_t strLen = 0;
        readBits((uint8_t*)&strLen, 8);
        if ((strLen & 0x80) != 0) {
            strLen &= 0x7F;
        } else {
            // NOTE: Swapped byte ordering
            strLen = (strLen << 8);
            readBits((uint8_t*)&strLen, 8);
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

	void readQuantitizedFloat(float& data, const size_t numBits, const float max, const float min = 0.0f) {
		long outBuf;
		readBits((uint8_t*)&outBuf, numBits);
		long field = static_cast<long>(outBuf);
		float range = max - min;
		if(range < -1.0f) {
			throw std::invalid_argument("quantitized float range - min value must never exceed max");
		}
		else if(range == 0.0f || field == 0L) {
			data = min;
		}
		else {
			long fieldMax = (1 << numBits) - 1;
			data = std::floor(static_cast<float>(field) * range / fieldMax + 0.5f);
			if(data < 0.0f) {
				data = 0.0f;
			}
			else if(data > range) {
				data = range;
			}
			data += min;
		}
	}

    /**
     * Templated write functions.
     */
    template<typename T, size_t arraySize>
    void write(const std::array<T, arraySize>& data) {
        static_assert(!std::is_pointer<T>::value, "Object type must not be a pointer.");

        writeBytes(data.data(), sizeof(T) * data.size());
    }

    template<typename T>
    void write(const std::vector<T>& data) {
        static_assert(!std::is_pointer<T>::value, "Object type must not be a pointer.");

        writeBytes(data.data(), sizeof(T) * data.size());
    }

    template<typename T>
    void write(const T& object) {
        static_assert(!std::is_pointer<T>::value, "Object type must not be a pointer.");

        writeBytes((const uint8_t*)&object, sizeof(T));
    }

    void writeStringLength(uint16_t length) {
        if (length < 128) {
            uint8_t strLenWithFlag = 0x80 | length;
            writeBytes((uint8_t*)&strLenWithFlag, 1);
        } else {
            // NOTE: Swapped byte ordering
            writeBytes(((uint8_t*)&length) + 1, 1);
            writeBytes((uint8_t*)&length, 1);
        }
    }

    void write(const std::string& str) {
        const uint16_t strLen = (uint16_t)str.length();
        writeStringLength(strLen);

        alignPos();

        writeBytes((uint8_t*)str.data(), strLen);
    }

    void write(const std::wstring& str) {
        const uint16_t strLen = (uint16_t)str.length();
        writeStringLength(strLen);

        alignPos();

        writeBytes((uint8_t*)str.data(), strLen * 2);
    }

    std::vector<uint8_t>& buf;

private:
    size_t streamBitPos;
    Error lastError;
};
