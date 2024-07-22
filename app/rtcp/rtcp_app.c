/* Standard includes. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* RTP includes. */
#include "rtcp_api.h"

#define RTCP_READ_UINT32    ( ctx.readWriteFunctions.readUint32Fn )
#define RTCP_HEADER_LENGTH                      4
#define RTCP_PACKET_TYPE_FIR                            192 /* https://datatracker.ietf.org/doc/html/rfc2032#section-5.2.1 */
#define RTCP_PACKET_TYPE_SENDER_REPORT                  200 /* https://datatracker.ietf.org/doc/html/rfc3550#section-6.4.1 */
#define RTCP_PACKET_TYPE_RECEIVER_REPORT                201 /* https://datatracker.ietf.org/doc/html/rfc3550#section-6.4.2 */
#define RTCP_PACKET_TYPE_SOURCE_DESCRIPTION             202
#define RTCP_PACKET_TYPE_TRANSPORT_SPECIFIC_FEEDBACK    205 /* https://datatracker.ietf.org/doc/html/rfc4585#section-6.2 */
#define RTCP_PACKET_TYPE_PAYLOAD_SPECIFIC_FEEDBACK      206 /* https://datatracker.ietf.org/doc/html/rfc4585#section-6.3 */

static void deserialize_test1( void )
{
    RtcpPacket_t rtcpPacket;
    RtcpContext_t ctx;
    RtcpResult_t result;

    memset( &rtcpPacket,
            0x00,
            sizeof( RtcpPacket_t ) );

    result = Rtcp_Init( &ctx );
    assert( RTCP_RESULT_OK == result );

    // Assert that we don't parse buffers that aren't even large enough
    uint8_t headerTooSmall[] = {0x00, 0x00, 0x00};
    result = Rtcp_DeserializePacket( &ctx,
                               headerTooSmall,
                               sizeof( headerTooSmall ),
                               &rtcpPacket );
    assert( RTCP_RESULT_INPUT_PACKET_TOO_SMALL == result );

    // Assert that we check version field
    uint8_t invalidVersionValue[] = {0x01, 0xcd, 0x00, 0x03, 0x2c, 0xd1, 0xa0, 0xde, 0x00, 0x00, 0xab, 0xe0, 0x00, 0xa4, 0x00, 0x00};
    result = Rtcp_DeserializePacket( &ctx,
                               invalidVersionValue,
                               sizeof( invalidVersionValue ),
                               &rtcpPacket );
    assert( RTCP_RESULT_WRONG_VERSION == result );

    // Assert that we check the length field
    uint8_t invalidLengthValue[] = {0x81, 0xcd, 0x00, 0x00, 0x2c, 0xd1, 0xa0, 0xde, 0x00, 0x00, 0xab, 0xe0, 0x00, 0xa4, 0x00, 0x00};
    result = Rtcp_DeserializePacket( &ctx,
                               invalidLengthValue,
                               sizeof( invalidLengthValue ),
                               &rtcpPacket );
    assert( RTCP_RESULT_OK == result );

    uint8_t validRtcpPacket[] = {0x81, 0xcd, 0x00, 0x03, 0x2c, 0xd1, 0xa0, 0xde, 0x00, 0x00, 0xab, 0xe0, 0x00, 0xa4, 0x00, 0x00};
    result = Rtcp_DeserializePacket( &ctx,
                               validRtcpPacket,
                               sizeof( validRtcpPacket ),
                               &rtcpPacket );
    assert( RTCP_RESULT_OK == result );

}
/*-----------------------------------------------------------*/

static void deserialize_test2( void )
{
    RtcpPacket_t rtcpPacket;
    RtcpContext_t ctx;
    RtcpResult_t result;

    memset( &rtcpPacket,
            0x00,
            sizeof( RtcpPacket_t ) );

    result = Rtcp_Init( &ctx );
    assert( RTCP_RESULT_OK == result );

    // Compound RTCP Packet that contains SR, SDES and REMB
    uint8_t compoundPacket[] = { 0x80, 0xc8, 0x00, 0x06, 0xf1, 0x2d, 0x7b, 0x4b, 0xe1, 0xe3, 0x20, 0x43, 0xe5, 0x3d, 0x10, 0x2b, 0xbf,
                                 0x58, 0xf7, 0xef, 0x00, 0x00, 0x23, 0xf3, 0x00, 0x6c, 0xd3, 0x75,
                                 0x81, 0xca, 0x00, 0x06, 0xf1, 0x2d, 0x7b, 0x4b, 0x01, 0x10, 0x2f, 0x76, 0x6d, 0x4b, 0x51, 0x6e, 0x47,
                                 0x6e, 0x55, 0x70, 0x4f, 0x2b, 0x70, 0x38, 0x64, 0x52, 0x00, 0x00,
                                 0x8f, 0xce, 0x00, 0x06, 0xf1, 0x2d, 0x7b, 0x4b, 0x00, 0x00, 0x00, 0x00, 0x52, 0x45, 0x4d, 0x42, 0x02,
                                 0x12, 0x2d, 0x97, 0x0c, 0xef, 0x37, 0x0d, 0x2d, 0x07, 0x3d, 0x1d };

    int currentOffset = 0;
    result = Rtcp_DeserializePacket( &ctx,
                               compoundPacket + currentOffset,
                               sizeof( compoundPacket ) - currentOffset,
                               &rtcpPacket );
    assert( RTCP_RESULT_OK == result );
    assert( rtcpPacket.header.packetType == RTCP_PACKET_SENDER_REPORT );

    currentOffset += ( rtcpPacket.payloadLength + RTCP_HEADER_LENGTH );
    result = Rtcp_DeserializePacket( &ctx,
                               compoundPacket + currentOffset,
                               sizeof( compoundPacket ) - currentOffset,
                               &rtcpPacket );
    assert( RTCP_RESULT_OK == result );
    assert( rtcpPacket.header.packetType == RTCP_PACKET_UNKNOWN );

    currentOffset += ( rtcpPacket.payloadLength + RTCP_HEADER_LENGTH );
    result = Rtcp_DeserializePacket( &ctx,
                               compoundPacket + currentOffset,
                               sizeof( compoundPacket ) - currentOffset,
                               &rtcpPacket );
    assert( RTCP_RESULT_OK == result );
    assert( rtcpPacket.header.packetType == RTCP_PACKET_PAYLOAD_FEEDBACK_REMB );
    currentOffset += ( rtcpPacket.payloadLength + RTCP_HEADER_LENGTH );
    assert( currentOffset == sizeof( compoundPacket ) );
}
/*-----------------------------------------------------------*/

void deserialize_rembValueGet()
{
    RtcpPacket_t rtcpPacket;
    RtcpContext_t ctx;
    RtcpResult_t result;
    RtcpRembPacket_t rembPacket;
    double maximumBitRate = 0;
    uint32_t * pSsrcList1, * pSsrcList2;
    uint8_t bufferNoUniqueIdentifier[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    //STATUS_RTCP_INPUT_REMB_INVALID
    uint8_t singleSSRC[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x52, 0x45, 0x4d, 0x42, 0x01, 0x12, 0x76, 0x28, 0x6c, 0x76, 0xe8, 0x55 };
    uint8_t multipleSSRC[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x52, 0x45, 0x4d, 0x42,
                               0x02, 0x12, 0x76, 0x28, 0x6c, 0x76, 0xe8, 0x55, 0x42, 0x42, 0x42, 0x42 };
    uint8_t invalidSSRCLength[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x52, 0x45,
                                    0x4d, 0x42, 0xFF, 0x12, 0x76, 0x28, 0x6c, 0x76, 0xe8, 0x55 }; 
    //STATUS_RTCP_INPUT_REMB_INVALID

    memset( &rtcpPacket,
            0x00,
            sizeof( RtcpPacket_t ) );

    result = Rtcp_Init( &ctx );
    assert( RTCP_RESULT_OK == result );

    rembPacket.ssrcListLength = 20;
    rembPacket.pSsrcList = malloc (rembPacket.ssrcListLength);

    rtcpPacket.pPayload = bufferNoUniqueIdentifier;
    rtcpPacket.payloadLength = sizeof( bufferNoUniqueIdentifier );
    rtcpPacket.header.packetType = RTCP_PACKET_PAYLOAD_FEEDBACK_REMB;

    result = Rtcp_ParseRembPacket( &ctx,
                                   &rtcpPacket,
                                   &rembPacket );
    assert( result == RTCP_RESULT_MALFORMED_PACKET );

    rtcpPacket.pPayload = singleSSRC;
    rtcpPacket.payloadLength = sizeof( singleSSRC );
    result = Rtcp_ParseRembPacket( &ctx,
                                   &rtcpPacket,
                                   &rembPacket );
    assert( RTCP_RESULT_OK == RTCP_RESULT_OK );
    maximumBitRate = rembPacket.bitRateMantissa << rembPacket.bitRateExponent;
    assert( rembPacket.ssrcListLength == 1 );
    assert( maximumBitRate == 2581120.0 );
    assert( rembPacket.pSsrcList[0] == 0x6c76e855 );

    rembPacket.ssrcListLength = 20;
    rtcpPacket.pPayload = multipleSSRC;
    rtcpPacket.payloadLength = sizeof( multipleSSRC );
    result = Rtcp_ParseRembPacket( &ctx,
                                   &rtcpPacket,
                                   &rembPacket );
    maximumBitRate = rembPacket.bitRateMantissa << rembPacket.bitRateExponent;
    assert( RTCP_RESULT_OK == RTCP_RESULT_OK );
    assert( rembPacket.ssrcListLength == 2 );
    assert( maximumBitRate == 2581120.0 );
    assert( rembPacket.pSsrcList[0] == 0x6c76e855 );
    assert( rembPacket.pSsrcList[1] == 0x42424242 );

    rtcpPacket.pPayload = invalidSSRCLength;
    rtcpPacket.payloadLength = sizeof( invalidSSRCLength );
    result = Rtcp_ParseRembPacket( &ctx,
                                   &rtcpPacket,
                                   &rembPacket );
    assert( RTCP_RESULT_INPUT_REMB_PACKET_INVALID == result );
}
/*-----------------------------------------------------------*/

void deserialize_senderReport()
{
    RtcpPacket_t rtcpPacket;
    RtcpContext_t ctx;
    RtcpResult_t result;
    RtcpSenderReport_t senderReport;
    uint8_t payload[] = { 0x2c, 0x38, 0xaf, 0xd2, 0xe9, 0xf8, 0x11, 0x68, 0x33, 0x33, 0xe8,
                          0x64, 0x00, 0x03, 0x77, 0xca, 0x00, 0x00, 0x01, 0x4c, 0x00, 0x01, 0x0b, 0x2f };

    memset( &senderReport,
            0x00,
            sizeof( RtcpSenderReport_t ) );

    result = Rtcp_Init( &ctx );
    assert( RTCP_RESULT_OK == result );

    rtcpPacket.pPayload = payload;
    rtcpPacket.payloadLength = sizeof( payload );
    rtcpPacket.header.packetType = RTCP_PACKET_SENDER_REPORT ;
    rtcpPacket.header.receptionReportCount = 0;
    
    result = Rtcp_ParseSenderReport( &ctx,
                                     &rtcpPacket,
                                     &senderReport );
    assert( RTCP_RESULT_OK == result );

    assert( senderReport.senderSsrc == 0x2c38afd2 );
    assert( senderReport.senderInfo.ntpTime == 0xe9f811683333e864 );
    assert( senderReport.senderInfo.rtpTime == 0x377ca );
    assert( senderReport.senderInfo.packetCount == 0x14c );
    assert( senderReport.senderInfo.octetCount == 0x10b2f );
}
/*-----------------------------------------------------------*/

void deserialize_receiverReport()
{
    RtcpPacket_t rtcpPacket;
    RtcpContext_t ctx;
    RtcpResult_t result;
    RtcpReceiverReport_t receiverReport;

    uint8_t payload[] = { 0x12, 0x34, 0x56, 0x78, 0x87, 0x65, 0x43, 0x21, 0x25, 0x00, 0x00, 0x01, // Fraction lost (25 in hex, approximately 10%)
                          0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x05 };

    memset( &receiverReport,
            0x00,
            sizeof( RtcpReceiverReport_t ) );

    result = Rtcp_Init( &ctx );
    assert( RTCP_RESULT_OK == result );

    rtcpPacket.pPayload = payload;
    rtcpPacket.payloadLength = sizeof( payload );
    rtcpPacket.header.packetType = RTCP_PACKET_RECEIVER_REPORT;
    rtcpPacket.header.receptionReportCount = 1;

    receiverReport.numReceptionReports = 1;
    receiverReport.pReceptionReports = malloc( sizeof(RtcpReceptionReport_t) );
    result = Rtcp_ParseReceiverReport( &ctx,
                                       &rtcpPacket,
                                       &receiverReport );
    assert( RTCP_RESULT_OK == result );

    assert( receiverReport.senderSsrc == 0x12345678 );
    assert( receiverReport.pReceptionReports->sourceSsrc == 0x87654321 );
    assert( receiverReport.pReceptionReports->fractionLost == 0x25 );
    assert( receiverReport.pReceptionReports->cumulativePacketsLost == 1 );
    assert( receiverReport.pReceptionReports->extendedHighestSeqNumReceived == 2 );
    assert( receiverReport.pReceptionReports->interArrivalJitter == 3 );
    assert( receiverReport.pReceptionReports->lastSR == 4 );
    assert( receiverReport.pReceptionReports->delaySinceLastSR == 5 );
}
/*-----------------------------------------------------------*/

void deserialize_nackPacket()
{
    RtcpPacket_t rtcpPacket;
    RtcpContext_t ctx;
    RtcpResult_t result;
    RtcpNackPacket_t nackPacket;

    // Assert that NACK list meets the minimum length requirement
    uint8_t nackListTooSmall[] = {0x00, 0x00, 0x00};
    uint8_t nackListMalformed[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t singlePID[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0xa8, 0x00, 0x00 };
    uint8_t compound[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0xa8, 0x00, 0x00, 0x0c, 0xff, 0x00, 0x02};

    result = Rtcp_Init( &ctx );
    assert( RTCP_RESULT_OK == result );

    /* nackListTooSmall Packet parsing */
    memset( &nackPacket,
            0x00,
            sizeof( RtcpNackPacket_t ) );
    rtcpPacket.header.packetType = RTCP_PACKET_TRANSPORT_FEEDBACK_NACK;

    rtcpPacket.pPayload = nackListTooSmall;
    rtcpPacket.payloadLength = sizeof(nackListTooSmall);
    result = Rtcp_ParseNackPacket( &ctx,
                                   &rtcpPacket,
                                   &nackPacket );
    assert( RTCP_RESULT_BAD_PARAM == result );

    /* nackListMalformed Packet parsing */
    rtcpPacket.pPayload = nackListTooSmall;
    rtcpPacket.payloadLength = sizeof(nackListTooSmall);
    result = Rtcp_ParseNackPacket( &ctx,
                                   &rtcpPacket,
                                   &nackPacket );
    assert( RTCP_RESULT_BAD_PARAM == result );

    /* singlePID Packet parsing */
    rtcpPacket.pPayload = singlePID;
    rtcpPacket.payloadLength = sizeof(singlePID);
    result = Rtcp_ParseNackPacket( &ctx,
                                   &rtcpPacket,
                                   &nackPacket );
    assert( RTCP_RESULT_OK == result );
    assert( nackPacket.seqNumListLength == 1 );

    nackPacket.pSeqNumList = malloc( nackPacket.seqNumListLength * sizeof( uint16_t ) );
    result = Rtcp_ParseNackPacket( &ctx,
                                   &rtcpPacket,
                                   &nackPacket );
    assert( RTCP_RESULT_OK == result );
    assert( nackPacket.pSeqNumList[0] == 3240 );
    free( nackPacket.pSeqNumList );
    nackPacket.pSeqNumList = NULL;

    /* compound Packet parsing */
    rtcpPacket.pPayload = compound;
    rtcpPacket.payloadLength = sizeof(compound);
    result = Rtcp_ParseNackPacket( &ctx,
                                   &rtcpPacket,
                                   &nackPacket );
    assert( RTCP_RESULT_OK == result );
    assert( nackPacket.seqNumListLength == 3 );

    nackPacket.pSeqNumList = malloc( nackPacket.seqNumListLength * sizeof( uint16_t ) );
    result = Rtcp_ParseNackPacket( &ctx,
                                   &rtcpPacket,
                                   &nackPacket );
    assert( RTCP_RESULT_OK == result );
    assert( nackPacket.pSeqNumList[0] == 3240 );
    assert( nackPacket.pSeqNumList[1] == 3327 );
    assert( nackPacket.pSeqNumList[2] == 3329 );
    free( nackPacket.pSeqNumList );
}
/*-----------------------------------------------------------*/

void serialize_senderReport()
{
    RtcpPacket_t rtcpPacket;
    RtcpContext_t ctx;
    RtcpResult_t result;
    RtcpSenderReport_t senderReport;
    size_t paylaodLength = sizeof( RtcpSenderReport_t ), bufferLen = paylaodLength + RTCP_HEADER_LENGTH;
    uint8_t * pBuffer;
    uint8_t expectedBuff[] = { 0x80, 0xc8, 0x00, 0x06, 0x2c, 0x38, 0xaf, 0xd2, 0xe9, 0xf8, 0x11, 0x68, 0x33, 0x33, 0xe8,
                               0x64, 0x00, 0x03, 0x77, 0xca, 0x00, 0x00, 0x01, 0x4c, 0x00, 0x01, 0x0b, 0x2f };

    memset( &rtcpPacket,
            0x00,
            sizeof( RtcpPacket_t ) );
    memset( &senderReport,
            0x00,
            sizeof( RtcpSenderReport_t ) );

    rtcpPacket.pPayload = malloc( paylaodLength );
    pBuffer = malloc( bufferLen );

    result = Rtcp_Init( &ctx );
    assert( RTCP_RESULT_OK == result );

    senderReport.numReceptionReports = 0;
    senderReport.senderSsrc = 0x2c38afd2;

    senderReport.senderInfo.ntpTime = 0xe9f811683333e864;
    senderReport.senderInfo.rtpTime = 0x377ca;
    senderReport.senderInfo.octetCount = 0x10b2f;
    senderReport.senderInfo.packetCount = 0x14c;

    result = Rtcp_SerializeSenderReport( &ctx,
                             &senderReport,
                             pBuffer,
                             &bufferLen );

    for( int i = 0; i < bufferLen; i++ )
    {
        assert( pBuffer[i] == expectedBuff[i] );
    }

}
/*-----------------------------------------------------------*/

/**
 * @brief Validate RTCP twcc packet parsing containing only RTCP RUN_LENGTH_CHUNK.
 */
void twccParseTwccPacket( void )
{
    RtcpContext_t ctx;
    RtcpResult_t result;
    RtcpPacket_t rtcpPacket;
    RtcpTwccPacket_t twccPacket;
    uint8_t twccpayload[] = { 0x1c, 0x8c, 0x77, 0xb6, 0x3a, 0x1b, 0x46, 0x4a, 0x00, 0x11, 0x00, 0x08, 0x63, 0xe3,
                              0x21, 0x01, 0x20, 0x08, 0xb3, 0x57, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4b, 0x00, 0x02 };
    uint16_t expectedSeqNumList[] = {17, 18, 19, 20, 21, 22, 23, 24};

    result = Rtcp_Init( &ctx );
    assert( RTCP_RESULT_OK == result );

    memset( &twccPacket,
            0x00,
            sizeof( RtcpTwccPacket_t ) );

    rtcpPacket.header.packetType = RTCP_PACKET_TRANSPORT_FEEDBACK_TWCC;
    rtcpPacket.pPayload = twccpayload;
    rtcpPacket.payloadLength = sizeof(twccpayload);
    twccPacket.arrivalInfoListLength = 0;
    twccPacket.pArrivalInfoList = NULL;
    result = Rtcp_ParseTwccPacket( &ctx,
                                   &rtcpPacket,
                                   &twccPacket );
    assert( RTCP_RESULT_OK == result );
    assert( twccPacket.arrivalInfoListLength == 8);
    twccPacket.pArrivalInfoList = malloc( twccPacket.arrivalInfoListLength * sizeof(PacketArrivalInfo_t) );

    result = Rtcp_ParseTwccPacket( &ctx,
                                   &rtcpPacket,
                                   &twccPacket );
    assert( RTCP_RESULT_OK == result );
    assert( twccPacket.baseSeqNum == 17 );

    for( int i = 0; i < twccPacket.arrivalInfoListLength ; i++ )
    {
        assert( expectedSeqNumList[i] == twccPacket.pArrivalInfoList[i].seqNum );
    }
}
/*-----------------------------------------------------------*/

/**
 * @brief Validate RTCP twcc packet parsing containing only RTCP RUN_LENGTH_CHUNK.
 */
void twccParseTwccPacket2( void )
{
    RtcpContext_t ctx;
    RtcpResult_t result;
    RtcpPacket_t rtcpPacket;
    RtcpTwccPacket_t twccPacket;
    uint8_t twccpayload[] = { 0xcb, 0x00, 0x18, 0x1a, 0x6d, 0x06, 0xec, 0xda, 0x00, 0xa5, 0x00, 0x0c, 0x64, 0x5e, 0x11, 0x0f, 0x20,
                              0x0c, 0x84, 0x00, 0x00, 0x00, 0x00, 0x23, 0x50, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02 };
    uint16_t expectedSeqNumList[] = { 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176 };

    result = Rtcp_Init( &ctx );
    assert( RTCP_RESULT_OK == result );

    memset( &twccPacket,
            0x00,
            sizeof( RtcpTwccPacket_t ) );

    rtcpPacket.header.packetType = RTCP_PACKET_TRANSPORT_FEEDBACK_TWCC;
    rtcpPacket.pPayload = twccpayload;
    rtcpPacket.payloadLength = sizeof(twccpayload);
    twccPacket.arrivalInfoListLength = 0;
    twccPacket.pArrivalInfoList = NULL;
    result = Rtcp_ParseTwccPacket( &ctx,
                                   &rtcpPacket,
                                   &twccPacket );
    assert( RTCP_RESULT_OK == result );
    assert( twccPacket.arrivalInfoListLength == 12);
    twccPacket.pArrivalInfoList = malloc( twccPacket.arrivalInfoListLength * sizeof(PacketArrivalInfo_t) );

    result = Rtcp_ParseTwccPacket( &ctx,
                                   &rtcpPacket,
                                   &twccPacket );
    assert( RTCP_RESULT_OK == result );
    assert( twccPacket.baseSeqNum == 165 );

    for( int i = 0; i < twccPacket.arrivalInfoListLength ; i++ )
    {
        assert( expectedSeqNumList[i] == twccPacket.pArrivalInfoList[i].seqNum );
    }
}
/*-----------------------------------------------------------*/

/**
 * @brief Validate RTCP twcc packet parsing containing both RTCP RUN_LENGTH_CHUNK,
 *        & RTCP STATUS_VECTOR_CHUNK.
 */
void twccParseTwccPacket3( void )
{
    RtcpContext_t ctx;
    RtcpResult_t result;
    RtcpPacket_t rtcpPacket;
    RtcpTwccPacket_t twccPacket;
    uint8_t twccpayload[] = { 0xf2, 0x54, 0x58, 0xd3, 0x1f, 0x00, 0xc3, 0xe8, 0x2e, 0x1b, 0x00, 0x13, 0x7b, 0xa3, 0x64, 0xf1, 0x9f, 0xff, 0x20, 0x05,
                              0x4e, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x52, 0x3e, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x11, 0x00, 0x02 };
    uint16_t expectedSeqNumList[] = { 11803, 11804, 11805, 11806, 11807, 11808, 11809, 11810, 11811, 11812, 11813, 11814, 11815, 11816, 11817, 11818, 11819, 11820, 11821 };

    result = Rtcp_Init( &ctx );
    assert( RTCP_RESULT_OK == result );

    memset( &twccPacket,
            0x00,
            sizeof( RtcpTwccPacket_t ) );

    rtcpPacket.header.packetType = RTCP_PACKET_TRANSPORT_FEEDBACK_TWCC;
    rtcpPacket.pPayload = twccpayload;
    rtcpPacket.payloadLength = sizeof(twccpayload);
    twccPacket.arrivalInfoListLength = 0;
    twccPacket.pArrivalInfoList = NULL;
    result = Rtcp_ParseTwccPacket( &ctx,
                                   &rtcpPacket,
                                   &twccPacket );
    assert( RTCP_RESULT_OK == result );
    assert( twccPacket.arrivalInfoListLength == 19 );
    twccPacket.pArrivalInfoList = malloc( twccPacket.arrivalInfoListLength * sizeof(PacketArrivalInfo_t) );

    result = Rtcp_ParseTwccPacket( &ctx,
                                   &rtcpPacket,
                                   &twccPacket );
    assert( RTCP_RESULT_OK == result );
    assert( twccPacket.baseSeqNum == 11803 );

    for( int i = 0; i < twccPacket.arrivalInfoListLength ; i++ )
    {
        assert( expectedSeqNumList[i] == twccPacket.pArrivalInfoList[i].seqNum );
    }
}
/*-----------------------------------------------------------*/

int main( void )
{
    deserialize_test1();
    deserialize_test2();
    deserialize_rembValueGet();
    deserialize_senderReport();
    deserialize_receiverReport();
    deserialize_nackPacket();

    printf( "\nAll deserialize test PASS.\r\n" );

    serialize_senderReport();

    printf( "\nAll serialize test PASS.\r\n" );

    twccParseTwccPacket();
    twccParseTwccPacket2();
    twccParseTwccPacket3();

    printf( "\nAll TWCC parsing test PASS.\r\n" );

    return 0;
}

/*-----------------------------------------------------------*/