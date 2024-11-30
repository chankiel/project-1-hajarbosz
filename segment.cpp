#include <cstdint>
#include <cstring>
#include "segment.hpp"

/**
 * Generate a Segment containing a SYN packet
 */
Segment syn(uint32_t seqNum)
{
    Segment segment;
    segment.seqNum = seqNum;
    segment.flags.syn = 1;
    return segment;
}

/**
 * Generate a Segment containing an ACK packet
 */
Segment ack(uint32_t seqNum, uint32_t ackNum)
{
    Segment segment;
    segment.seqNum = seqNum;
    segment.ackNum = ackNum;
    segment.flags.ack = 1;
    return segment;
}

/**
 * Generate a Segment containing a SYN-ACK packet
 */
Segment synAck(uint32_t seqNum)
{
    Segment segment;
    segment.seqNum = seqNum;
    segment.flags.syn = 1;
    segment.flags.ack = 1;
    return segment;
}

/**
 * Generate a Segment containing a FIN packet
 */
Segment fin()
{
    Segment segment;
    segment.flags.fin = 1;
    return segment;
}

/**
 * Generate a Segment containing a FIN-ACK packet
 */
Segment finAck()
{
    Segment segment;
    segment.flags.fin = 1;
    segment.flags.ack = 1;
    return segment;
}

/**
 * Calculate the checksum for a given Segment
 */
uint8_t *calculateChecksum(Segment segment)
{
    segment.checksum = 0;

    size_t segmentSize = sizeof(Segment);
    size_t payloadSize = (segment.data_offset - 5) * 4;
    size_t totalSize = segmentSize + payloadSize;

    uint8_t *buffer = new uint8_t[totalSize];
    memcpy(buffer, &segment, segmentSize);

    if (segment.payload != nullptr && payloadSize > 0)
    {
        memcpy(buffer + segmentSize, segment.payload, payloadSize);
    }

    uint32_t sum = 0;
    for (size_t i = 0; i < totalSize; i += 2)
    {
        uint16_t word = (buffer[i] << 8);
        if (i + 1 < totalSize)
        {
            word |= buffer[i + 1];
        }
        sum += word;

        while (sum >> 16)
        {
            sum = (sum & 0xFFFF) + (sum >> 16);
        }
    }

    uint16_t checksum = ~sum;
    // segment.checksum = checksum;

    uint8_t *result = new uint8_t[2];
    result[0] = (checksum >> 8) & 0xFF;
    result[1] = checksum & 0xFF;

    delete[] buffer;
    return result;
}


/**
 * Update a Segment with the calculated checksum
 */
Segment updateChecksum(Segment segment)
{
    auto checksum = calculateChecksum(segment);
    segment.checksum = (checksum[0] << 8) | checksum[1];
    return segment;
}

/**
 * Verify if a Segment has a valid checksum
 */
bool isValidChecksum(Segment segment)
{
    auto checksum = calculateChecksum(segment);
    uint16_t computed = (checksum[0] << 8) | checksum[1];
    return computed == segment.checksum;
}