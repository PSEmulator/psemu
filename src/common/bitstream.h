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

    BitStream(std::vector<uint8_t>&);

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
    void setPos(size_t);

    /**
     * Moves the stream pos by some delta.
     */
    void deltaPos(int32_t);

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
    void readBytes(uint8_t*, size_t, bool = false);

    /**
     * @return The boolean value of the next bit in the stream
     */
    bool readBit(bool = false);

    /**
     * Reads a number of bits.
     * NOTE: WILL clobber any existing data in the bytes that get written to
     */
    void readBits(uint8_t*, size_t, bool = false);

    /**
     * Writes a number of bytes.
     */
    void writeBytes(const uint8_t*, size_t);

    /**
     * Writes a boolean value as a single bit.
     */
    void writeBit(bool);

    /**
     * Writes a number of bits.
     */
    void writeBits(const uint8_t*, size_t);

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

    void read(std::string&);

    void read(std::wstring&);

    /**
    Read from buffer a float value as an integer with a 0-max range as a ratio to the float's min-max range.
    @param data the float value to write to
    @param numBits the number of bits to be occupied in the buffer, the max converted integer value
    @param max the maximum float value
    @param min the minimum float value, defaulting to 0.0f
    @throw invalid_argument if min is greater than max
    */
    void readQuantitizedFloat(float&, const size_t, const float, const float = 0.0f);

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

    void writeStringLength(uint16_t);

    void write(const std::string&);

    void write(const std::wstring&);

    /**
    Write to buffer a float value within a min-max range as an integer value within a 0-max range.
    @param data the float to write to buffer
    @param numBits the number of bits to be occupied in the buffer, the max converted integer value
    @param max the maximum float value
    @param min the minimum float value, defaulting to 0.0f
    @throw invalid_argument if min is greater than max
    */
    void writeQuantitizedFloat(const float&, const size_t, const float, const float = 0.0f);

    std::vector<uint8_t>& buf;

private:
    size_t streamBitPos;
    Error lastError;
};
