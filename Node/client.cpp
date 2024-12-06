#include "client.hpp"
#include "../Socket/socket.hpp"
#include "../tools/tools.hpp"
#include <cstdlib> // For malloc and free
#include <random>
#include <stdexcept>
#include <string>
int CLIENT_BROADCAST_TIMEOUT = 12; // temporary
int CLIENT_COMMON_TIMEOUT = 12;    // temporary
int CLIENT_MAX_TRY = 10;

ConnectionResult Client::findBroadcast(string dest_ip, uint16_t dest_port)
{
  connection->setBroadcast();

  std::cout << dest_ip << " " << dest_port << std::endl;
  for (int i = 0; i < CLIENT_MAX_TRY; i++)
  {
    try
    {
      // Segment temp = createSegment("AWD",port,dest_port);
      Segment temp = broad();

      connection->sendSegment(temp, dest_ip, dest_port);
      commandLine('i', "Sending Broadcast\n");
      Message answer =
          connection->consumeBuffer("", 0, 0, 0, 255, CLIENT_BROADCAST_TIMEOUT);
      commandLine('i', "Someone received the broadcast\n");
      return ConnectionResult(true, answer.ip, answer.port, answer.segment.seqNum,
                              answer.segment.ackNum);
    }
    catch (const std::runtime_error &e)
    {
      commandLine('x', "Timeout " + std::to_string(i + 1) + "\n");
      continue;
    }
  }
  return ConnectionResult(false, 0, 0, 0, 0);
}

ConnectionResult Client::startFin(string dest_ip, uint16_t dest_port,
                                  uint32_t seqNum,uint32_t ackNum) {
  for (int i = 0; i < CLIENT_MAX_TRY; i++) {
    try {
      // OLD
      // // Send FIN
      // std::cout<<seqNum<<" "<<ackNum<<endl;
      // Segment finSeg = fin(seqNum,ackNum);
      // connection->sendSegment(finSeg, dest_ip, dest_port);
      // commandLine('i', "[Closing] [S=" + to_string(finSeg.seqNum) + "] Sending FIN request to " + dest_ip + ":" +
      //                      to_string(dest_port) + "\n");
      // // REC ACK
      // Message answer_fin = connection->consumeBuffer(
      //     dest_ip, dest_port, ackNum, seqNum + 1, ACK_FLAG, CLIENT_COMMON_TIMEOUT);
      // commandLine('+', "[Closing] [S=" + to_string(answer_fin.segment.seqNum) + "] [A="+ to_string(answer_fin.segment.ackNum) + "] Received ACK request from  " + dest_ip +
      //                      to_string(dest_port) + "\n");

      // // REC FIN
      // Message fin2 = connection->consumeBuffer(dest_ip, dest_port, ackNum, seqNum + 1,
      //                                          FIN_FLAG, CLIENT_COMMON_TIMEOUT);
      // commandLine('+', "[Closing] [S=" + to_string(fin2.segment.seqNum) + "] [A="+ to_string(fin2.segment.ackNum) + "] Received ACK request from  " + dest_ip +
      //                      to_string(dest_port) + "\n");
      // // commandLine('+', "[Closing] Received FIN request from  " + dest_ip +
      // //                      to_string(dest_port) + "\n");

      // // Send ACK
      // Segment ackSeg = ack(seqNum + 1, ackNum+ 2);
      // ackSeg.seqNum += sizeof(ackSeg);
      // commandLine('i', "[Closing] [A=" + to_string(ackSeg.seqNum) + " ] Sending FIN request to " + dest_ip + ":" +
      //                 to_string(dest_port) + "\n");
      // // commandLine('i', "[Closing] Sending ACK request to " + dest_ip +
      // //                      to_string(dest_port) + "\n");

      // commandLine('i', "Connection Closed\n");

      // NEW 
      // Send FIN
      // std::cout<<seqNum<<" "<<ackNum<<endl;
      Segment finSeg = fin(seqNum,ackNum); 
      connection->sendSegment(finSeg, dest_ip, dest_port);
      commandLine('i', "[Closing] [S=" + to_string(finSeg.seqNum) + "] [A=" + to_string(finSeg.ackNum) +  "] Sending FIN request to " + dest_ip + ":" +
                           to_string(dest_port) + "\n");
      // REC ACK
      Message answer_fin = connection->consumeBuffer(
          dest_ip, dest_port, ackNum, seqNum + 1, ACK_FLAG, CLIENT_COMMON_TIMEOUT);
      commandLine('+', "[Closing] [S=" + to_string(answer_fin.segment.seqNum) + "] [A="+ to_string(answer_fin.segment.ackNum) + "] Received ACK request from  " + dest_ip +
                           to_string(dest_port) + "\n");

      // REC FIN
      Message fin2 = connection->consumeBuffer(dest_ip, dest_port, ackNum, seqNum + 1,
                                               FIN_FLAG, CLIENT_COMMON_TIMEOUT);
      commandLine('+', "[Closing] [S=" + to_string(fin2.segment.seqNum) + "] [A="+ to_string(fin2.segment.ackNum) + "] Received ACK request from  " + dest_ip +
                           to_string(dest_port) + "\n");
      // commandLine('+', "[Closing] Received FIN request from  " + dest_ip +
      //                      to_string(dest_port) + "\n");

      // Send ACK
      Segment ackSeg = ack(seqNum + 1, ackNum+ 2);
      ackSeg.seqNum += sizeof(ackSeg);
      commandLine('i', "[Closing] [A=" + to_string(ackSeg.seqNum) + " ] Sending FIN request to " + dest_ip + ":" +
                      to_string(dest_port) + "\n");
      // commandLine('i', "[Closing] Sending ACK request to " + dest_ip +
      //                      to_string(dest_port) + "\n");

      commandLine('i', "Connection Closed\n");

      return ConnectionResult(true, dest_ip, dest_port,0,0);
    } catch (const std::runtime_error &e)
    {
      commandLine('x', "Timeout " + std::to_string(i + 1) + "\n");
    }
  }
  return ConnectionResult(false, dest_ip, dest_port,0,0);
}

ConnectionResult Client::startHandshake(string dest_ip, uint16_t dest_port)
{
  uint32_t r_seq_num = generateRandomNumber(10, 4294967295);

  commandLine('i', "Sender Program's Three Way Handshake\n");

  Segment synSegment = syn(r_seq_num);

  for (int i = 0; i < 10; i++) {
    try {
      // Send syn?
      connection->sendSegment(synSegment, dest_ip, dest_port);
      connection->setSocketState(TCPState::SYN_SENT);

      commandLine('i', "[Established] [Seg 1] [S=" + std::to_string(r_seq_num) + "] Sending SYN request to " + dest_ip + ":" + std::to_string(dest_port));

      // Wait syn-ack?
      Message result = connection->consumeBuffer(dest_ip, dest_port, 0, r_seq_num + 1, SYN_ACK_FLAG, 10);
      Segment synAckSegment = result.segment;
      // std::cout << synAckSegment.seqNum << " " << spynAckSegment.ackNum << std::endl;
      connection->setSocketState(TCPState::ESTABLISHED);

      commandLine('~', "[Established] Waiting for segments to be ACKed");
      commandLine('i', "[Established] [Seg 1] [S=" + std::to_string(synAckSegment.seqNum) + "] [A=" + std::to_string(synAckSegment.ackNum) + "] Received SYN-ACK request from " + result.ip + ":" + std::to_string(result.port));

      // Send ack?
      uint32_t ackNum = synAckSegment.seqNum + 1;
      Segment ackSegment = ack(r_seq_num + 1, ackNum);
      connection->sendSegment(ackSegment, dest_ip, dest_port);
      commandLine('i', "[Established] [Seg 2] [A=" + std::to_string(ackNum) + "] Sending ACK request to " + dest_ip + ":" + std::to_string(dest_port));
      commandLine('~', "Ready to receive input from " + dest_ip + ":" + std::to_string(dest_port) + "\n");
      return ConnectionResult(true, dest_ip, dest_port, synAckSegment.seqNum, synAckSegment.ackNum);
    }
    catch (const std::exception &e)
    {
      commandLine('e', "[Handshake] Attempt failed: " + std::string(e.what()) + "\n");
    }
  }
  commandLine('e', "[Handshake] Failed after 10 retries\n");
  return ConnectionResult(false, dest_ip, dest_port, 0, 0);
}

void Client::run()
{
  connection->listen();
  connection->startListening();
  ConnectionResult statusBroadcast = findBroadcast("255.255.255.255", serverPort);
  ConnectionResult statusHandshake = startHandshake(statusBroadcast.ip,statusBroadcast.port);
  ConnectionResult statusFin = startFin(statusBroadcast.ip,statusBroadcast.port,statusHandshake.ackNum,statusHandshake.seqNum+1);
  if (statusBroadcast.success)
  {
    std::cout << "SUCCESS" << std::endl;
  }
  else
  {
    std::cout << "FALSE" << std::endl;
  }
}