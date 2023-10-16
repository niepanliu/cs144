#pragma once

#include "byte_stream.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"

class Timer 
{
public:
  bool start = false;
  uint64_t ROT = 0;
  uint64_t nowROT = 0;
  uint64_t time_ms = 0;

  Timer(uint64_t initROT);
  bool is_overtime();
  bool startTimer();
  bool stopTimer();
  void tick(uint64_t ms_since_last_tick );
};

class TCPSender
{
  Wrap32 isn_;
  bool syn = false;
  bool fin = false;
  uint64_t ack_sqn = 0;
  uint64_t next_sqn = 0;
  uint64_t window_size = 1;
  uint64_t initial_RTO_ms_ {};
  uint64_t retransmitions = 0;
  uint64_t flight_cnt = 0;
  //TCPSenderMessage msg;

  std::deque<TCPSenderMessage> messages_queue {};
  std::deque<TCPSenderMessage> messages_flight {};

  Timer time {initial_RTO_ms_};

public:
  /* Construct TCP sender with given default Retransmission Timeout and possible ISN */
  TCPSender( uint64_t initial_RTO_ms, std::optional<Wrap32> fixed_isn );

  /* Push bytes from the outbound stream */
  void push( Reader& outbound_stream );

  /* Send a TCPSenderMessage if needed (or empty optional otherwise) */
  std::optional<TCPSenderMessage> maybe_send();

  /* Generate an empty TCPSenderMessage */
  TCPSenderMessage send_empty_message() const;

  /* Receive an act on a TCPReceiverMessage from the peer's receiver */
  void receive( const TCPReceiverMessage& msg );

  /* Time has passed by the given # of milliseconds since the last time the tick() method was called. */
  void tick( uint64_t ms_since_last_tick );

  /* Accessors for use in testing */
  uint64_t sequence_numbers_in_flight() const;  // How many sequence numbers are outstanding?
  uint64_t consecutive_retransmissions() const; // How many consecutive *re*transmissions have happened?
};


