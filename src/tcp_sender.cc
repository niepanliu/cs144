#include "tcp_sender.hh"
#include "tcp_config.hh"

#include <random>
#include <iostream>

using namespace std;

Timer::Timer(uint64_t initROT) {
  ROT = initROT;
  nowROT = initROT;
  time_ms = 0;
}

bool Timer::is_overtime()
{
  return time_ms >= nowROT;
}

bool Timer::startTimer()
{
  start = true;
  time_ms = 0;
  return start;
}

void Timer::tick(uint64_t ms_since_last_tick )
{
  if(start)
    time_ms += ms_since_last_tick;
  // printf("%ld\n",time_ms);
  // printf("%ld\n",ROT);
  // printf("%ld\n",nowROT);
  
}

bool Timer::stopTimer()
{
  start = false;
  return start;
}

/* TCPSender constructor (uses a random ISN if none given) */
TCPSender::TCPSender( uint64_t initial_RTO_ms, optional<Wrap32> fixed_isn )
  : isn_( fixed_isn.value_or( Wrap32 { random_device()() } ) ), initial_RTO_ms_( initial_RTO_ms )
{}

uint64_t TCPSender::sequence_numbers_in_flight() const
{
  return flight_cnt;
}

uint64_t TCPSender::consecutive_retransmissions() const
{
  return retransmitions;
}

optional<TCPSenderMessage> TCPSender::maybe_send()
{
  //printf("maybe send:");
  if(messages_queue.empty()){
    //printf("no\n");
    return {};
  }
  //printf("yes");
  if(!time.start)
    time.startTimer();
  TCPSenderMessage msg = messages_queue.front();
  messages_queue.pop_front();
  return msg;
}

void TCPSender::push( Reader& outbound_stream )
{
  //printf("%ld\n", window_size);
  uint64_t cur_windows = window_size != 0 ? window_size : 1;
  //printf("%ld\n",cur_windows);
  while (flight_cnt < cur_windows)
  {
    TCPSenderMessage msg;
    if(!syn) {
      msg.SYN = true;
      syn = true;
      flight_cnt ++;
    }
    msg.seqno =  Wrap32::wrap(next_sqn, isn_);
    uint64_t payload_size = min(TCPConfig::MAX_PAYLOAD_SIZE, cur_windows - flight_cnt);
    read(outbound_stream, payload_size, msg.payload);
    flight_cnt += msg.payload.size();
    if(!fin && outbound_stream.is_finished() && flight_cnt < cur_windows) {
      fin = true;
      msg.FIN = true;
      flight_cnt ++;
    }
    if(msg.sequence_length() == 0)
      break;

    messages_queue.push_back(msg);
    next_sqn += msg.sequence_length();
    messages_flight.push_back(msg);

    if(msg.FIN || outbound_stream.bytes_buffered() == 0)
      break;
  }

}

TCPSenderMessage TCPSender::send_empty_message() const
{
  Wrap32 sqn = Wrap32::wrap(next_sqn, isn_);
  return {sqn, false, {}, false};
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  window_size = msg.window_size;
  if(msg.ackno.has_value()) {
    ack_sqn = msg.ackno.value().unwrap(isn_, next_sqn);
    if(ack_sqn > next_sqn)
      return;
  
    
    while(!messages_flight.empty()) {
      TCPSenderMessage front_msg = messages_flight.front();
      if(front_msg.seqno.unwrap(isn_, next_sqn) + front_msg.sequence_length() <= ack_sqn) {
        flight_cnt -= front_msg.sequence_length();
        messages_flight.pop_front();
        time.nowROT = time.ROT;
        if(!messages_flight.empty()) 
          time.startTimer();
        retransmitions = 0;
      } else 
        break;

      if(messages_flight.empty())
        time.stopTimer();
    }
  }

}

void TCPSender::tick( const size_t ms_since_last_tick )
{
  time.tick(ms_since_last_tick);
  if(time.is_overtime()) {
    messages_queue.push_back(messages_flight.front());
    if(window_size != 0) {
      time.nowROT *= 2;
      retransmitions ++;
    }
    time.startTimer();
  }
}

