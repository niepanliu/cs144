#include "tcp_receiver.hh"
#include <iostream>

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message, Reassembler& reassembler, Writer& inbound_stream )
{
  if(warp.has_value() == false) {
    if(message.SYN == false)
      return ;
    warp = message.seqno;
  }
  uint64_t checkpoint = inbound_stream.bytes_pushed() + 1;
  uint64_t abs_seqno = message.seqno.unwrap(warp.value(), checkpoint);
  uint64_t first_index = message.SYN ? 0 : abs_seqno - 1;
  if(first_index == UINT64_MAX){
    return ;
  }
  reassembler.insert(first_index, message.payload, message.FIN, inbound_stream);
}

TCPReceiverMessage TCPReceiver::send( const Writer& inbound_stream ) const
{
  // Your code here.
  TCPReceiverMessage message {};
  message.window_size = inbound_stream.available_capacity() < UINT16_MAX ? inbound_stream.available_capacity() : UINT16_MAX;

  if(warp.has_value()) {
    uint64_t abs_ack = inbound_stream.bytes_pushed() + 1 + inbound_stream.is_closed();
    message.ackno = Wrap32::wrap(abs_ack, warp.value());
  }
  return message;
}
