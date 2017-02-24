#include <iostream>
#include "bitstream.h"
#include "log.h"
#include "test.h"
#include "util.h"

class PlanetsideBitstream {
public:
    std::vector<uint8_t>& pData;
    int32_t streamPos;

    PlanetsideBitstream(std::vector<uint8_t>& pData) :
        pData(pData),
        streamPos(0) {

    }
};

void Write(PlanetsideBitstream *pThis, const uint8_t *pData, int numBits) {
    int remainingBits; // ebx@1
    PlanetsideBitstream *this_; // ebp@1
    int byteOffset; // eax@2
    int bitOffset; // ecx@2
    int bitsLeft; // edx@4
    int v8; // ebx@7
    int v9; // edi@7
    unsigned __int8 *this_pData; // ecx@9

    remainingBits = numBits;
    this_ = pThis;
    while (1) {
        byteOffset = this_->streamPos / 8;
        bitOffset = this_->streamPos % 8;
        if (!bitOffset && !(remainingBits & 7)) {
            memcpy(
                &this_->pData[byteOffset],
                pData,
                4 * ((unsigned int)remainingBits >> 5) + (((unsigned int)remainingBits >> 3) & 3));
            this_->streamPos += remainingBits;
            return;
        }
        bitsLeft = 8 - bitOffset;
        if (!bitOffset)
            this_->pData[byteOffset] = 0;
        if (remainingBits <= (unsigned int)bitsLeft)
            break;
        v8 = remainingBits - bitsLeft;
        v9 = v8;
        if (v8 >= this_->streamPos % 8)
            v8 = this_->streamPos % 8;
        this_pData = this_->pData.data();
        if ((unsigned int)bitsLeft >= 8)
            this_pData[byteOffset] = *pData;
        else
            this_pData[byteOffset] |= *pData >> v8;
        this_->streamPos += bitsLeft;
        if (v8)
            Write(this_, pData, v8);
        if (v9 == v8)
            return;
        remainingBits = v9 - v8;
        ++pData;
    }
    this_->pData[byteOffset] |= *pData << (bitsLeft - remainingBits);
    this_->streamPos += remainingBits;
}

void testBitstreamWriteBitsBasic() {
    static std::vector<uint8_t> expectedBuf = std::vector<uint8_t>({
        0x0B, 0xCA
    });

    std::vector<uint8_t> bitstreamBuf;
    BitStream bitstream(bitstreamBuf);

    uint32_t zero = 0;
    bitstream.writeBits((const uint8_t*)&zero, 4);

    uint32_t abc = 0xABC;
    bitstream.writeBits((const uint8_t*)&abc, 12);

    assertBuffersEqual(bitstreamBuf, expectedBuf);
}

void testBitstreamWriteBits() {
    std::vector<uint8_t> psBitstreamBuf(32);
    PlanetsideBitstream psBitstream(psBitstreamBuf);

    std::vector<uint8_t> bitstreamBuf(32);
    BitStream bitstream(bitstreamBuf);

    for (int32_t n = 1; n < 32; ++n) {
        memset(psBitstreamBuf.data(), 0x0, 32);
        psBitstream.streamPos = n;
        Write(&psBitstream, (const uint8_t*)&n, n);

        memset(bitstreamBuf.data(), 0x0, 32);
        bitstream.setPos(n);
        bitstream.writeBits((const uint8_t*)&n, n);

        assertBuffersEqual(psBitstreamBuf, bitstreamBuf);
    }
}

void testBitstreamReadBits() {
    std::vector<uint8_t> psBitstreamBuf(32);
    PlanetsideBitstream psBitstream(psBitstreamBuf);

    BitStream bitstream(psBitstreamBuf);

    for (int32_t n = 1; n < 32; ++n) {
        memset(psBitstreamBuf.data(), 0x0, 32);
        psBitstream.streamPos = n;
        Write(&psBitstream, (const uint8_t*)&n, n);

        int32_t readResult = 0;
        bitstream.setPos(n);
        bitstream.readBits((uint8_t*)&readResult, n);

        assertEqual(readResult, n);
    }
}

void testBitstreamReadQuantitizedFloat() {
    static std::vector<uint8_t> expectedBuf = std::vector<uint8_t>({
        0x6C, 0x2D, 0x76, 0x55, 0x35, 0xCA, 0x16 //6C2D7 65535 CA16
    });
    BitStream bitstream(expectedBuf);

    double x;
    double y;
    double z;
    bitstream.readQuantitizedDouble(x, 20, 8192.0);
    bitstream.readQuantitizedDouble(y, 20, 8192.0);
    bitstream.readQuantitizedDouble(z, 16, 1024.0);
    assertEqual(compareDecimals(x, 3674.85, 0.01), 0);
    assertEqual(compareDecimals(y, 2726.7917, 0.01), 0);
    assertEqual(compareDecimals(z, 91.1581, 0.01), 0);
}

void testBitstreamWriteQuantitizedFloat() {
    static std::vector<uint8_t> expectedBuf = std::vector<uint8_t>({
        0x6C, 0x2D, 0x76, 0x55, 0x35, 0xCA, 0x16 //6C2D7 65535 CA16
    });
    BitStream expected_bitstream(expectedBuf);

    double a = 3674.85;
    double b = 2726.7917;
    double c = 91.1581;
    std::vector<uint8_t> bitstreamBuf;
    BitStream test_bitstream(bitstreamBuf);
    test_bitstream.writeQuantitizedDouble(a, 20, 8192.0);
    test_bitstream.writeQuantitizedDouble(b, 20, 8192.0);
    test_bitstream.writeQuantitizedDouble(c, 16, 1024.0);
    assertBuffersEqual(bitstreamBuf, expectedBuf);
}

void testBitstreamReadQuantitizedFloatLimits() {
    static std::vector<uint8_t> expectedBuf = std::vector<uint8_t>({
        0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF //FFFF 0000 FFFF
    });
    BitStream bitstream(expectedBuf);

    double a;
    double b;
    double c;
    bitstream.readQuantitizedDouble(a, 16, 256.0, -256.0);
    bitstream.readQuantitizedDouble(b, 16, 256.0, -256.0);
    bitstream.readQuantitizedDouble(c, 16, 256.0, -256.0);
    assertEqual(compareDecimals(a, 256.0, 0.01), 0);
    assertEqual(compareDecimals(b, -256.0, 0.01), 0);
    assertEqual(compareDecimals(c, 256.0, 0.01), 0);
}

void testBitstreamWriteQuantitizedFloatLimits() {
    static std::vector<uint8_t> expectedBuf = std::vector<uint8_t>({
        0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF //FFFF 0000 FFFF
    });
    BitStream test_bitstream(expectedBuf);

    double a = 260.5; //too high for 256.0
    double b = -260.5; //too low for -256.0
    std::vector<uint8_t> bitstreamBuf;
    BitStream bitstream(bitstreamBuf);
    bitstream.writeQuantitizedDouble(a, 16, 256.0, -256.0);
    bitstream.writeQuantitizedDouble(b, 16, 256.0, -256.0);
    bitstream.writeQuantitizedDouble(a, 16, 256.0, -256.0);
    assertBuffersEqual(bitstreamBuf, expectedBuf); //confirms limiting
}

void testBitStreamWriteAndReadBackQuantitizedFloats() {
    double a = 3674.85;
    double b = 2726.79;
    double c = 91.1421;

    std::vector<uint8_t> bitstreamBuf;
    BitStream bitstream(bitstreamBuf);
    bitstream.writeQuantitizedDouble(a, 20, 8192.0);
    bitstream.writeQuantitizedDouble(b, 20, 8192.0);
    bitstream.writeQuantitizedDouble(c, 16, 1024.0);

    bitstream.setPos(0); //rewind the stream
    double ax;
    double bx;
    double cx;
    bitstream.readQuantitizedDouble(ax, 20, 8192.0);
    bitstream.readQuantitizedDouble(bx, 20, 8192.0);
    bitstream.readQuantitizedDouble(cx, 16, 1024.0);
    assertEqual(compareDecimals(ax, a, 0.01), 0);
    assertEqual(compareDecimals(bx, b, 0.01), 0);
    assertEqual(compareDecimals(cx, c, 0.01), 0);
}

void testBitStreamReadAndWriteQuantitizedFloats() {
    static std::vector<uint8_t> expectedBuf = std::vector<uint8_t>({
        0x6C, 0x2D, 0x76, 0x55, 0x35, 0xCA, 0x16 //6C2D7 65535 CA16
    });
    BitStream test_bitstream(expectedBuf);

    double a;
    double b;
    double c;
    test_bitstream.readQuantitizedDouble(a, 20, 8192.0);
    test_bitstream.readQuantitizedDouble(b, 20, 8192.0);
    test_bitstream.readQuantitizedDouble(c, 16, 1024.0);

    std::vector<uint8_t> bitstreamBuf;
    BitStream bitstream(bitstreamBuf);
    bitstream.writeQuantitizedDouble(a, 20, 8192.0);
    bitstream.writeQuantitizedDouble(b, 20, 8192.0);
    bitstream.writeQuantitizedDouble(c, 16, 1024.0);
    assertBuffersEqual(bitstreamBuf, expectedBuf);
}

void testBitstream() {
    testBitstreamWriteBitsBasic();
    testBitstreamWriteBits();
    testBitstreamReadBits();

    // TODO: Test byte write/read
    // TODO: Test mixed bits and bytes write/read
    // TODO: Test primitive write/read
    // TODO: Test string write/read

    testBitstreamReadQuantitizedFloat();
    testBitstreamWriteQuantitizedFloat();
    testBitstreamReadQuantitizedFloatLimits();
    testBitstreamWriteQuantitizedFloatLimits();
    testBitStreamWriteAndReadBackQuantitizedFloats();
    testBitStreamReadAndWriteQuantitizedFloats();
}
