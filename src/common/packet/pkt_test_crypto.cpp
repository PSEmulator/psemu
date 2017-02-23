#include <iostream>
#include <vector>
#include "pkt_all.h"
#include "pkt_test.h"
#include "common/bitstream.h"
#include "common/log.h"
#include "common/test.h"

void testClientChallengeXchg() {
    static std::vector<uint8_t> encodedBuf = hexToBytes(
        "0101962D845324F5997CC7D16031D1F5 67E900010002FF2400001000F57511EB 8E5D1EFB8B7F3287D5A18B1710000000 00000000000000000000000000020000 010307000000");

    // Decode
    // TODO: Test missing properties
    BitStream decodeBitStream(encodedBuf);
    ClientChallengeXchg decodePacket = ClientChallengeXchg::decode(decodeBitStream);
    assertEqual(decodePacket.clientTime, 0x53842D96);
    static std::array<uint8_t, 12> expectedChallenge = { 0x24, 0xF5, 0x99, 0x7C, 0xC7, 0xD1, 0x60, 0x31, 0xD1, 0xF5, 0x67, 0xE9 };
    assertBuffersEqual(decodePacket.challenge, expectedChallenge);
    assertEqual(decodePacket.pLen, 0x0010);
    static std::array<uint8_t, 16> expectedP = { 0xF5, 0x75, 0x11, 0xEB, 0x8E, 0x5D, 0x1E, 0xFB, 0x8B, 0x7F, 0x32, 0x87, 0xD5, 0xA1, 0x8B, 0x17 };
    assertBuffersEqual(decodePacket.p, expectedP);
    assertEqual(decodePacket.gLen, 0x0010);
    static std::array<uint8_t, 16> expectedG = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02 };
    assertBuffersEqual(decodePacket.g, expectedG);
}

void testClientFinished() {
    static std::vector<uint8_t> encodedBuf = hexToBytes(
        "101000EDDC35F252B02D0E496BA27354 578E730114EA3CF05DA5CB42568BB91A A7");

    // Decode
    BitStream decodeBitStream(encodedBuf);
    ClientFinished decodePacket = ClientFinished::decode(decodeBitStream);
    assertEqual(decodePacket.pubKeyLen, 0x0010);
    static std::array<uint8_t, 16> expectedPubKey = { 0xED, 0xDC, 0x35, 0xF2, 0x52, 0xB0, 0x2D, 0x0E, 0x49, 0x6B, 0xA2, 0x73, 0x54, 0x57, 0x8E, 0x73 };
    assertBuffersEqual(decodePacket.pubKey, expectedPubKey);
    static std::array<uint8_t, 12> expectedChallengeResult = { 0xEA, 0x3C, 0xF0, 0x5D, 0xA5, 0xCB, 0x42, 0x56, 0x8B, 0xB9, 0x1A, 0xA7 };
    assertBuffersEqual(decodePacket.challengeResult, expectedChallengeResult);
}

void testServerChallengeXchg() {
    static std::vector<uint8_t> encodedBuf = hexToBytes(
        "0201962D84531B0E6408CD935EC2429A EB58000103070000000C00100051F83C E645E86C3E79C8FC70F6DDF14B0E");

    // Encode
    ServerChallengeXchg encodePacket;
    encodePacket.unk0 = 2;
    encodePacket.unk1 = 1;
    encodePacket.serverTime = 0x53842D96;
    encodePacket.challenge = { 0x1B, 0x0E, 0x64, 0x08, 0xCD, 0x93, 0x5E, 0xC2, 0x42, 0x9A, 0xEB, 0x58 };
    encodePacket.unkChallengeEnd = 0;
    encodePacket.unkObjects = 1;
    encodePacket.unk2 = { 0x03, 0x07, 0x00, 0x00, 0x00, 0x0C, 0x00 };
    encodePacket.pubKeyLen = 16;
    encodePacket.pubKey = { 0x51, 0xF8, 0x3C, 0xE6, 0x45, 0xE8, 0x6C, 0x3E, 0x79, 0xC8, 0xFC, 0x70, 0xF6, 0xDD, 0xF1, 0x4B };
    encodePacket.unk3 = 14;

    std::vector<uint8_t> testEncodingBuf;
    encodePacket.encode(BitStream(testEncodingBuf));
    assertBuffersEqual(testEncodingBuf, encodedBuf);
}

void testServerFinished() {
    static std::vector<uint8_t> encodedBuf = hexToBytes(
        "0114D64FFB8E526311B4AF46BECE");

    // Encode
    ServerFinished encodePacket;
    encodePacket.unk0 = 0x1401;
    encodePacket.challengeResult = { 0xD6, 0x4F, 0xFB, 0x8E, 0x52, 0x63, 0x11, 0xB4, 0xAF, 0x46, 0xBE, 0xCE };

    std::vector<uint8_t> testEncodingBuf;
    encodePacket.encode(BitStream(testEncodingBuf));
    assertBuffersEqual(testEncodingBuf, encodedBuf);
}

void testPacketCodingCrypto() {
    testClientChallengeXchg();
    testClientFinished();
    testServerChallengeXchg();
    testServerFinished();
}
