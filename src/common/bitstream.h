#pragma once

#include <vector>

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

    BitStream(std::vector<uint8_t>& exisitingBuf);

    /**
     * @return The total number of bits in the buffer
     */
    size_t getSizeBits() const;

    /**
     * @return The number of unread bits after the stream pos
     */
    size_t getRemainingBits() const;

    /**
        * @return The number of COMPLETELY unread bytes after the stream pos
        */
    size_t getRemainingBytes() const;

    /**
     * @return A pointer to the byte holding the stream pos
     */
    uint8_t* getHeadBytePtr() const;

    /**
     * @return An iterator to the byte holding the stream pos
     */
    std::vector<uint8_t>::iterator getHeadIterator() const;

    /**
     * @return A pointer to the byte holding the given bit pos
     */
    uint8_t* getPosBytePtr(size_t) const;

    /**
     * @return The current stream pos
     */
    size_t getPos() const;

    /**
     * Sets the current stream pos.
     */
    void setPos(size_t pos);

    /**
     * Moves the stream pos by some delta.
     */
    void deltaPos(int32_t delta);

    /**
     * Aligns the stream pos to the next highest byte boundary if necessary.
     */
    void alignPos();

    /**
     * @return The error type if the stream has encountered a serialization error.
     */
    Error getLastError();

    /**
     * Reads a number of bytes.
     */
    void readBytes(uint8_t* outBuf, size_t numBytes, bool peek = false);

    /**
     * @return The boolean value of the next bit in the stream
     */
    bool readBit(bool peek = false);

    /**
     * Reads a number of bits.
     * NOTE: WILL clobber any existing data in the bytes that get written to
     */
    void readBits(uint8_t* outBuf, size_t numBits, bool peek = false);

    /**
     * Writes a number of bytes.
     */
    void writeBytes(const uint8_t* data, size_t numBytes);

    /**
     * Writes a boolean value as a single bit.
     */
    void writeBit(bool value);

    /**
     * Writes a number of bits.
     */
    void writeBits(const uint8_t* data, size_t numBits);

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

    uint16_t readStringLength();

    void read(std::string& str);

    void read(std::wstring& str);

    /**
    Read from buffer a double value within a min-max float range as a value in a 0-[max] integer range.
    @param data the double value to write to
    @param numBits the number of bits to be occupied in the buffer, the max converted integer value
    @param max the maximum float value
    @param min the minimum float value, defaulting to 0.0f
    @param epsilon a comparable tolerable difference between decimal numbers, defaulting to 0.001
    @throw invalid_argument if min is greater than max
    @throw invalid_argument if the number of bits is zero
    */
    void readQuantitizedDouble(double& data, size_t numBits, float max, float min = 0.0f, double epsilon = 0.001);

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

    void writeStringLength(uint16_t length);

    void write(const std::string& str);

    void write(const std::wstring& str);

    /**
    Write to buffer a double value within a min-max float range as a value in a 0-[max] integer range.
    @param data the double to write to buffer
    @param numBits the number of bits to be occupied in the buffer, the max converted integer value
    @param max the maximum float value
    @param min the minimum float value, defaulting to 0.0f
    @param epsilon a comparable tolerable difference between decimal numbers, defaulting to 0.001
    @throw invalid_argument if min is greater than max
    @throw invalid_argument if the number of bits is zero
    */
    void writeQuantitizedDouble(const double& data, size_t numBits, float max, float min = 0.0f, double epsilon = 0.001);

    std::vector<uint8_t>& buf;

private:
    size_t streamBitPos;
    Error lastError;
};
