#include "socket.hpp"
#include <chrono>
#include <iostream>

TCPSocket::TCPSocket(const string &ip, int port) : localIP(ip), localPort(port), isListening(false)
{
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        throw std::runtime_error("Socket creation failed.");
    }
    socketState = TCPState::CLOSED;
}

TCPSocket::~TCPSocket()
{
    stopListening();
    close();
}

sockaddr_in TCPSocket::createSockAddr(const string &ipAddress, int port)
{
    sockaddr_in address = {};
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    if (inet_pton(AF_INET, ipAddress.c_str(), &address.sin_addr) <= 0)
    {
        throw std::runtime_error("Invalid IP address format.");
    }
    return address;
}

bool TCPSocket::send(const string &destinationIP, int32_t destinationPort, void *data, uint32_t size)
{
    auto destAddress = createSockAddr(destinationIP, destinationPort);
    if (sendto(sockfd, data, size, 0, (struct sockaddr *)&destAddress, sizeof(destAddress)) < 0)
    {
        return false;
    }
    return true;
}

void TCPSocket::sendSegment(const Segment &segment, const string &destinationIP, uint16_t destinationPort)
{
    auto updatedSegment = updateChecksum(segment);
    uint32_t segmentSize = updatedSegment.payloadSize + 24;

    auto *buffer = new uint8_t[segmentSize];
    encodeSegment(updatedSegment, buffer);

    send(destinationIP, destinationPort, buffer, segmentSize);
    delete[] buffer;
}

int32_t TCPSocket::receive(void *buffer, uint32_t bufferSize, bool peek)
{
    sockaddr_in sourceAddress = {};
    socklen_t addressLength = sizeof(sourceAddress);

    int flags = peek ? MSG_PEEK : 0;
    return recvfrom(sockfd, buffer, bufferSize, flags, (struct sockaddr *)&sourceAddress, &addressLength);
}

void TCPSocket::produceBuffer()
{ 
    while (isListening)
    {
        try
        {
            uint8_t *dataBuffer = new uint8_t[MAX_SEGMENT_SIZE];
            sockaddr_in clientAddress = {};
            socklen_t addressLength = sizeof(clientAddress);

            int bytesRead = recvfrom(sockfd, dataBuffer, MAX_SEGMENT_SIZE, 0, (struct sockaddr *)&clientAddress, &addressLength);
            if (bytesRead <= 0)
            {
                delete[] dataBuffer;
                if (!isListening)
                    break;
                continue;
            }

            Segment segment = decodeSegment(dataBuffer, bytesRead);
            delete[] dataBuffer;

            if (!isValidChecksum(segment))
            {
                continue;
            }

            Message message(inet_ntoa(clientAddress.sin_addr), ntohs(clientAddress.sin_port), segment);

            {
                lock_guard<mutex> lock(bufferMutex);
                packetBuffer.push_back(std::move(message));
                bufferCondition.notify_one();
            }
        }
        catch (const std::exception &ex)
        {
            if (isListening)
            {
                std::cerr << "Error in producer: " << ex.what() << "\n";
            }
        }
    }
}

Message TCPSocket::consumeBuffer(const string &filterIP, uint16_t filterPort,
                                  uint32_t filterSeqNum, uint32_t filterAckNum,
                                  uint8_t filterFlags, int timeout)
{
    auto start = std::chrono::steady_clock::now();
    auto timeoutPoint = (timeout > 0) ? start + std::chrono::seconds(timeout) : std::chrono::steady_clock::time_point::max();

    while (isListening)
    {
        std::unique_lock<mutex> lock(bufferMutex);
        bufferCondition.wait_for(lock, std::chrono::milliseconds(100), [this]() { return !packetBuffer.empty(); });

        for (auto it = packetBuffer.begin(); it != packetBuffer.end(); ++it)
        {
            const auto &msg = *it;
            if ((filterIP.empty() || msg.ip == filterIP) &&
                (filterPort == 0 || msg.port == filterPort) &&
                (filterSeqNum == 0 || msg.segment.seqNum == filterSeqNum) &&
                (filterAckNum == 0 || msg.segment.ackNum == filterAckNum) &&
                (filterFlags == 0 || getFlags8(&msg.segment) == filterFlags))
            {
                Message result = std::move(*it);
                packetBuffer.erase(it);
                return result;
            }
        }

        if (timeout > 0 && std::chrono::steady_clock::now() > timeoutPoint)
        {
            throw std::runtime_error("Buffer consumer timeout.");
        }
    }

    throw std::runtime_error("Socket is no longer listening.");
}

void TCPSocket::setSocketState(TCPState newState)
{
    socketState = newState;
}

TCPState TCPSocket::getSocketState() const
{
    return socketState;
}

void TCPSocket::startListening()
{
    isListening = true;
    listenerThread = std::thread(&TCPSocket::produceBuffer, this);
}

void TCPSocket::stopListening()
{
    isListening = false;
    if (listenerThread.joinable())
    {
        listenerThread.join();
    }
}

void TCPSocket::close()
{
  if (sockfd >= 0)
  {
    ::close(sockfd);
    sockfd = -1;
    std::cout << "Socket closed" << std::endl;
  }
}