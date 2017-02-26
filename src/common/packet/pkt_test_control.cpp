#include <iostream>
#include <vector>
#include "pkt_all.h"
#include "pkt_test.h"
#include "common/bitstream.h"
#include "common/log.h"
#include "common/test.h"

void testControlSync() {
    static std::vector<uint8_t> encodedBuf = hexToBytes(
        "0007 5268 0000004D 00000052 0000004D 0000007C 0000004D 0000000000000276 0000000000000275");

    // Decode
    // TODO: Are these actually big endian?
    BitStream decodeBitStream(encodedBuf);
    assertControlOpcode(decodeBitStream, OP_ControlSync);
    ControlSync decodePacket = ControlSync::decode(decodeBitStream);
    assertEqual(decodePacket.timeDiff, 0x6852);
    assertEqual(decodePacket.unk, 0x4D000000);
    assertEqual(decodePacket.field1, 0x52000000);
    assertEqual(decodePacket.field2, 0x4D000000);
    assertEqual(decodePacket.field3, 0x7C000000);
    assertEqual(decodePacket.field4, 0x4D000000);
    assertEqual(decodePacket.field64A, 0x7602000000000000);
    assertEqual(decodePacket.field64B, 0x7502000000000000);
}

void testControlSyncResp() {
    static std::vector<uint8_t> encodedBuf = hexToBytes(
        "0008 5268 21392D92 0000000000000276 0000000000000275 0000000000000275 0000000000000276");

    // Encode
    // TODO: Are these actually big endian?
    ControlSyncResp encodePacket;
    encodePacket.timeDiff = 0x6852;
    encodePacket.serverTick = 0x922D3921;
    encodePacket.field1 = 0x7602000000000000;
    encodePacket.field2 = 0x7502000000000000;
    encodePacket.field3 = 0x7502000000000000;
    encodePacket.field4 = 0x7602000000000000;

    std::vector<uint8_t> testEncodingBuf;
    encodePacket.encode(BitStream(testEncodingBuf));
    assertBuffersEqual(testEncodingBuf, encodedBuf);
}

void testClientStart() {
    static std::vector<uint8_t> encodedBuf = hexToBytes(
        "0001 0000000200261E27000001F0");

    // Decode
    BitStream decodeBitStream(encodedBuf);
    assertControlOpcode(decodeBitStream, OP_ClientStart);
    ClientStart decodePacket = ClientStart::decode(decodeBitStream);
    assertEqual(decodePacket.unk0, 0x02000000);
    assertEqual(decodePacket.clientNonce, 0x271E2600);
    assertEqual(decodePacket.unk1, 0xF0010000);
}

void testServerStart() {
    static std::vector<uint8_t> encodedBuf = hexToBytes(
        "0002 00261E2751BDC1CE000000000001D300 000002");

    // Encode
    ServerStart encodePacket;
    encodePacket.clientNonce = 0x271E2600;
    encodePacket.serverNonce = 0xCEC1BD51;
    encodePacket.unk0 = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0xD3, 0x00, 0x00, 0x00, 0x02 };

    std::vector<uint8_t> testEncodingBuf;
    encodePacket.encode(BitStream(testEncodingBuf));
    assertBuffersEqual(testEncodingBuf, encodedBuf);
}

void testSlottedMetaAck() {
    uint8_t expectedSlot = 2;
    uint8_t expectedOpcode = OP_RelatedB0 + expectedSlot % 4;
    static std::vector<uint8_t> encodedBuf = std::vector<uint8_t>({
        0x00, expectedOpcode, 0x34, 0x12
    });

    // Encode
    SlottedMetaAck encodePacket;
    encodePacket.slot = expectedSlot;
    encodePacket.subslot = 0x1234;

    std::vector<uint8_t> testEncodingBuf;
    encodePacket.encode(BitStream(testEncodingBuf));
    assertBuffersEqual(testEncodingBuf, encodedBuf);
}

void testPacketCodingControl() {
    testControlSync();
    testControlSyncResp();
    testClientStart();
    testServerStart();
    testSlottedMetaAck();
    // TODO: Test SlottedMetaPacket
}
