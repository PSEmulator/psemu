#include <algorithm>
#include "bitstream.h"
#include "util.h"

#define BITS_TO_BYTES(bits) (((bits) + 7) / 8)


BitStream::BitStream(std::vector<uint8_t>& exisitingBuf) :
    buf(exisitingBuf),
    streamBitPos(0),
    lastError(Error::NONE) {

}

size_t BitStream::getSizeBits() const {
    return buf.size() * 8;
}

size_t BitStream::getRemainingBits() const {
    size_t sizeBits = getSizeBits();
    if (streamBitPos >= sizeBits) {
        return 0;
    } else {
        return sizeBits - streamBitPos;
    }
}

size_t BitStream::getRemainingBytes() const {
    size_t usedBytes = BITS_TO_BYTES(streamBitPos);
    if (usedBytes >= buf.size()) {
        return 0;
    } else {
        return buf.size() - usedBytes;
    }
}

uint8_t* BitStream::getHeadBytePtr() const {
    return buf.data() + (streamBitPos / 8);
}

std::vector<uint8_t>::iterator BitStream::getHeadIterator() const {
    return buf.begin() + (streamBitPos / 8);
}

uint8_t* BitStream::getPosBytePtr(size_t bitPos) const {
    return buf.data() + (bitPos / 8);
}

size_t BitStream::getPos() const {
    return streamBitPos;
}

void BitStream::setPos(size_t pos) {
    if (pos > buf.size() * 8) {
        lastError = Error::INVALID_STREAM_POS;
        return;
    }

    streamBitPos = pos;
}

void BitStream::deltaPos(int32_t delta) {
    // TODO: Signedness is funky here. Figure out a better way.
    int32_t newPos = (int32_t)streamBitPos + delta;

    if (newPos < 0 || newPos > buf.size() * 8) {
        lastError = Error::INVALID_STREAM_POS;
        return;
    }

    streamBitPos = newPos;
}

void BitStream::alignPos() {
    size_t bitsIn = (streamBitPos % 8);
    if (bitsIn != 0) {
        deltaPos(8 - bitsIn);
    }
}

BitStream::Error BitStream::getLastError() {
    return lastError;
}

void BitStream::readBytes(uint8_t* outBuf, size_t numBytes, bool peek) {
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

bool BitStream::readBit(bool peek) {
    size_t remainingBits = getRemainingBits();
    if (remainingBits < 1) {
        lastError = Error::READ_TOO_MUCH;
        return false;
    }

    bool result = (*getHeadBytePtr() & (0x80 >> (streamBitPos & 0x7))) != 0;

    streamBitPos++;

    return result;
}

void BitStream::readBits(uint8_t* outBuf, size_t numBits, bool peek) {
    //NOTE: WILL CLOBBER any existing data in the bytes that get written to
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

void BitStream::writeBytes(const uint8_t* data, size_t numBytes) {
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

void BitStream::writeBit(bool value) {
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

void BitStream::writeBits(const uint8_t* data, size_t numBits) {
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

uint16_t BitStream::readStringLength() {
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

void BitStream::read(std::string& str) {
    uint16_t strLen = readStringLength();

    alignPos();

    str.resize(strLen);
    readBytes((uint8_t*)str.data(), strLen);
}

void BitStream::read(std::wstring& str) {
    uint16_t strLen = readStringLength();

    alignPos();

    str.resize(strLen);
    readBytes((uint8_t*)str.data(), strLen * 2);
}

void BitStream::readQuantitizedDouble(double& data, size_t numBits, float max, float min, double epsilon) {
    double range = static_cast<double>(max - min);
    if (range + epsilon < 0.0) {
        throw std::invalid_argument("quantitized double range - min value is greater than max value");
    }
    else if (numBits == 0) {
        throw std::invalid_argument("quantitized double size - no bits, no value");
    }
    uint64_t outBuf = 0L;
    readBits((uint8_t*)&outBuf, numBits);
    if (range < epsilon || outBuf == 0L) {
        data = min;
    } else {
        uint64_t fieldMax = (1 << numBits) - 1;
        data = outBuf * range / fieldMax;
        if (data + epsilon < 0.0) {
            data = 0.0;
        } else if (data + epsilon > range) {
            data = range;
        }
        data += min;
    }
}

void BitStream::writeStringLength(uint16_t length) {
    if (length < 128) {
        uint8_t strLenWithFlag = 0x80 | length;
        writeBytes((uint8_t*)&strLenWithFlag, 1);
    } else {
        // NOTE: Swapped byte ordering
        writeBytes(((uint8_t*)&length) + 1, 1);
        writeBytes((uint8_t*)&length, 1);
    }
}

void BitStream::write(const std::string& str) {
    const uint16_t strLen = (uint16_t)str.length();
    writeStringLength(strLen);

    alignPos();

    writeBytes((uint8_t*)str.data(), strLen);
}

void BitStream::write(const std::wstring& str) {
    const uint16_t strLen = (uint16_t)str.length();
    writeStringLength(strLen);

    alignPos();

    writeBytes((uint8_t*)str.data(), strLen * 2);
}

void BitStream::writeQuantitizedDouble(const double& data, size_t numBits, float max, float min, double epsilon) {
    double range = static_cast<double>(max - min);
    uint64_t inBuf;
    if (range + epsilon < 0.0) {
        throw std::invalid_argument("quantitized double range - min value is greater than max value");
    } else if (numBits == 0) {
        throw std::invalid_argument("quantitized double size - no bits, no value");
    } else
    if (std::fabs(range) < epsilon || data + epsilon <= min) {
        inBuf = 0L;
    } else {
        uint64_t fieldMax = (1 << numBits) - 1;
        inBuf = static_cast<uint64_t>(((data - min) * fieldMax / range));
        if (inBuf > fieldMax) {
            inBuf = fieldMax;
        }
    }
    writeBits((uint8_t*)&inBuf, numBits);
}
