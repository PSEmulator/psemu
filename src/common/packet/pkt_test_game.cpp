#include <iostream>
#include <vector>
#include "pkt_all.h"
#include "pkt_test.h"
#include "common/bitstream.h"
#include "common/log.h"
#include "common/test.h"

void testCharacterInfoMessage() {
    // TODO: Replace with actual packet data
    static std::vector<uint8_t> encodedBuf = hexToBytes(
        "14 21A575B7453423123254769834128CB3 C1A980");

    // Encode
    CharacterInfoMessage encodePacket;
    encodePacket.unknown = 0xB775A521;
    encodePacket.zoneId = 0x12233445;
    encodePacket.charId = 0x98765432;
    encodePacket.charGUID = 0x1234;
    encodePacket.finished = true;
    encodePacket.secondsSinceLastLogin = 0x53836719;

    std::vector<uint8_t> testEncodingBuf;
    encodePacket.encode(BitStream(testEncodingBuf));
    assertBuffersEqual(testEncodingBuf, encodedBuf);
}

void testCharacterRequestMessage() {
    // TODO: Replace with actual packet data
    static std::vector<uint8_t> encodedBuf = hexToBytes(
        "30 4534231232547698");

    // Decode
    BitStream decodeBitStream(encodedBuf);
    assertOpcode(decodeBitStream, OP_CharacterRequestMessage);
    CharacterRequestMessage decodePacket = CharacterRequestMessage::decode(decodeBitStream);
    assertEqual(decodePacket.charId, 0x12233445);
    assertEqual(decodePacket.action, 0x98765432);
}

void testConnectToWorldMessage() {
    static std::vector<uint8_t> encodedBuf = hexToBytes(
        "04 8667656D696E69  8C36342E33372E3135382E36393C75");

    // Encode
    ConnectToWorldMessage encodePacket;
    encodePacket.serverName = "gemini";
    encodePacket.serverAddress = "64.37.158.69";
    encodePacket.serverPort = 30012;

    std::vector<uint8_t> testEncodingBuf;
    encodePacket.encode(BitStream(testEncodingBuf));
    assertBuffersEqual(testEncodingBuf, encodedBuf);
}

void testConnectToWorldRequestMessage() {
    // TODO: Doesnt seem like a very good test case...
    static std::vector<uint8_t> encodedBuf = hexToBytes(
        "03 8667656D696E69 0000000000000000 00000000 00000000 00000000 00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  80 00 00");

    // Decode
    BitStream decodeBitStream(encodedBuf);
    assertOpcode(decodeBitStream, OP_ConnectToWorldRequestMessage);
    ConnectToWorldRequestMessage decodePacket = ConnectToWorldRequestMessage::decode(decodeBitStream);
    assertEqual(decodePacket.serverName, "gemini");
    static std::array<uint8_t, 32> expectedToken = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    assertBuffersEqual(decodePacket.token, expectedToken);
    assertEqual(decodePacket.majorVersion, 0x00000000);
    assertEqual(decodePacket.minorVersion, 0x00000000);
    assertEqual(decodePacket.revision, 0x00000000);
    assertEqual(decodePacket.buildDate, "");
    assertEqual(decodePacket.unk0, 0);
}

void testKeepAliveMessage() {
    static std::vector<uint8_t> encodedBuf = hexToBytes(
        "BA 3412");

    // Encode
    KeepAliveMessage encodePacket;
    encodePacket.keepAliveCode = 0x1234;

    std::vector<uint8_t> testEncodingBuf;
    encodePacket.encode(BitStream(testEncodingBuf));
    assertBuffersEqual(testEncodingBuf, encodedBuf);

    // Decode
    BitStream decodeBitStream(encodedBuf);
    assertOpcode(decodeBitStream, OP_KeepAliveMessage);
    KeepAliveMessage decodePacket = KeepAliveMessage::decode(decodeBitStream);
    assertEqual(decodePacket.keepAliveCode, 0x1234);
}

void testLoadMapMessage() {
    static std::vector<uint8_t> encodedBuf = hexToBytes(
        "31 85 6D61703130 83 7A3130 0FA0 19000000 F6 F1 60 86 80");

    // Encode
    LoadMapMessage encodePacket;
    encodePacket.mapName = "map10";
    encodePacket.navMapName = "z10";
    encodePacket.unk1 = 40975;
    encodePacket.unk2 = 25;
    encodePacket.weaponsUnlocked = true;
    encodePacket.checksum = 230810349;

    std::vector<uint8_t> testEncodingBuf;
    encodePacket.encode(BitStream(testEncodingBuf));
    assertBuffersEqual(testEncodingBuf, encodedBuf);
}

void testLoginMessage() {
    static std::vector<uint8_t> encodedBuf = hexToBytes(
        "01 030000000F0000008B44656320203220 32303039420061736466843132333454 000000");

    // Decode
    BitStream decodeBitStream(encodedBuf);
    assertOpcode(decodeBitStream, OP_LoginMessage);
    LoginMessage decodePacket = LoginMessage::decode(decodeBitStream);
    assertEqual(decodePacket.majorVersion, 3);
    assertEqual(decodePacket.minorVersion, 15);
    assertEqual(decodePacket.buildDate, "Dec  2 2009");
    assertEqual(decodePacket.credentialsType, LoginMessage::CT_UserPassword);
    assertEqual(decodePacket.username, "asdf");
    assertEqual(decodePacket.password, "1234");
    assertEqual(decodePacket.revision, 84);

    // TODO: Test decode with CT_UserToken
}

void testLoginRespMessage() {
    static std::vector<uint8_t> encodedBuf = hexToBytes(
        "02 5448495349534D59544F4B454E594553 0000000018FABE0C0000000000000000 0000000001000000020000006B7BD828 84617364661127000080");

    // Encode
    LoginRespMessage encodePacket;
    encodePacket.token = { 'T', 'H', 'I', 'S', 'I', 'S', 'M', 'Y', 'T', 'O', 'K', 'E', 'N', 'Y', 'E', 'S' };
    encodePacket.unk0 = { 0x00, 0x00, 0x00, 0x00, 0x18, 0xFA, 0xBE, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    encodePacket.error = 0;
    encodePacket.stationError = 1;
    encodePacket.subscriptionStatus = 2;
    encodePacket.unk1 = 685276011;
    encodePacket.username = "asdf";
    encodePacket.privilege = 10001;

    std::vector<uint8_t> testEncodingBuf;
    encodePacket.encode(BitStream(testEncodingBuf));
    assertBuffersEqual(testEncodingBuf, encodedBuf);
}

void testSetCurrentAvatarMessage() {
    // TODO: Doesnt seem like a very good test case...
    static std::vector<uint8_t> encodedBuf = hexToBytes(
        "32 4B0000");

    // Encode
    SetCurrentAvatarMessage encodePacket;
    encodePacket.guid = 75;
    encodePacket.unk1 = 0;
    encodePacket.unk2 = 0;

    std::vector<uint8_t> testEncodingBuf;
    encodePacket.encode(BitStream(testEncodingBuf));
    assertBuffersEqual(testEncodingBuf, encodedBuf);
}

void testAvatarFirstTimeEventMessage() {
    static std::vector<uint8_t> decodeBuf = hexToBytes(
        "69 4b00 c000 01000000 9e 766973697465645f63657274696669636174696f6e5f7465726d696e616c");

    //decode
    BitStream decodeBitStream(decodeBuf);
    assertOpcode(decodeBitStream, OP_AvatarFirstTimeEventMessage);
    AvatarFirstTimeEventMessage decodePacket = AvatarFirstTimeEventMessage::decode(decodeBitStream);
    assertEqual(decodePacket.avatar_uid, 75);
    assertEqual(decodePacket.object_id, 192);
    assertEqual(decodePacket.unk, 1);
    assertEqual(decodePacket.event_name, "visited_certification_terminal");
}

void testPacketCodingGame() {
    testCharacterInfoMessage();
    testCharacterRequestMessage();
    testConnectToWorldMessage();
    testConnectToWorldRequestMessage();
    testKeepAliveMessage();
    testLoadMapMessage();
    testLoginMessage();
    testLoginRespMessage();
    // TODO: Test ObjectCreateMessage
    testSetCurrentAvatarMessage();
    // TODO: Test VNLWorldStatusMessage
    testAvatarFirstTimeEventMessage();
}
