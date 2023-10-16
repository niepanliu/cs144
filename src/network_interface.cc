#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

using namespace std;

// ethernet_address: Ethernet (what ARP calls "hardware") address of the interface
// ip_address: IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( const EthernetAddress& ethernet_address, const Address& ip_address )
  : ethernet_address_( ethernet_address ), ip_address_( ip_address )
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address_ ) << " and IP address "
       << ip_address.ip() << "\n";
}
// dgram: the IPv4 datagram to be sent
// next_hop: the IP address of the interface to send it to (typically a router or default gateway, but
// may also be another host if directly connected to the same network as the destination)

// Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) by using the
// Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
  uint32_t ip = next_hop.ipv4_numeric();
  auto it =  ipToEther.find(ip);
  if(it != ipToEther.end()) {
    EthernetFrame temp{{ipToEther[ip].first, ethernet_address_, EthernetHeader::TYPE_IPv4}, serialize(dgram)};
    out_frame.push(std::move(temp));
  } else {
    auto arp_it = arp_time.find(ip);
    if(arp_it == arp_time.end()) {
      ARPMessage arpQuest;
      arpQuest.opcode = ARPMessage::OPCODE_REQUEST;
      arpQuest.target_ip_address = ip;
      arpQuest.sender_ip_address = ip_address_.ipv4_numeric();
      arpQuest.sender_ethernet_address = ethernet_address_;
      EthernetFrame temp {{ETHERNET_BROADCAST, ethernet_address_, EthernetHeader::TYPE_ARP}, serialize(arpQuest)};
      out_frame.push(std::move(temp));
      arp_time.emplace(ip, 0);
      wait_queue.insert({ip, {dgram}});
    } else
      wait_queue[ip].push_back(dgram);
  }  
}

// frame: the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame( const EthernetFrame& frame )
{
  if(frame.header.dst != ethernet_address_ && frame.header.dst != ETHERNET_BROADCAST)
    return {};
  if(frame.header.type == EthernetHeader::TYPE_IPv4) {
    InternetDatagram dgram;
    parse(dgram, frame.payload);
    return dgram;
  } else if (frame.header.type == EthernetHeader::TYPE_ARP) {
    ARPMessage msg;
    parse(msg, frame.payload);
    ipToEther.insert({msg.sender_ip_address, {msg.sender_ethernet_address, 0}});
    if(msg.opcode == ARPMessage::OPCODE_REQUEST) {
      if(ip_address_.ipv4_numeric() == msg.target_ip_address) {
        ARPMessage reply;
        reply.opcode = ARPMessage::OPCODE_REPLY;
        reply.target_ip_address = msg.sender_ip_address;
        reply.target_ethernet_address = msg.sender_ethernet_address;
        reply.sender_ethernet_address = ethernet_address_;
        reply.sender_ip_address = ip_address_.ipv4_numeric();
        EthernetFrame temp {{msg.sender_ethernet_address, ethernet_address_, EthernetHeader::TYPE_ARP}, serialize(reply)};
        out_frame.push(std::move(temp));
      }
    } else if (msg.opcode == ARPMessage::OPCODE_REPLY) {
      ipToEther.insert({msg.sender_ip_address, {msg.sender_ethernet_address, 0}});
      std::vector<InternetDatagram> dgram = wait_queue[msg.sender_ip_address];
      for(uint64_t i = 0; i < dgram.size(); i++)
        send_datagram(dgram[i], Address::from_ipv4_numeric(msg.sender_ip_address));
      wait_queue.erase(msg.sender_ip_address);
    }
  }
  return {};
}

// ms_since_last_tick: the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
  (void)ms_since_last_tick;
  uint64_t ip_ttl = 30000;
  uint64_t apr_ttl = 5000;

  for(auto it = ipToEther.begin(); it != ipToEther.end();) {
    it->second.second += ms_since_last_tick;
    if(it->second.second >= ip_ttl)
      it = ipToEther.erase(it);
    else
      it++;
  }

  for(auto it = arp_time.begin(); it != arp_time.end();) {
    it->second += ms_since_last_tick;
    if(it->second >= apr_ttl)
      it = arp_time.erase(it);
    else
      it++;
  }
}

optional<EthernetFrame> NetworkInterface::maybe_send()
{
  if(out_frame.empty())
    return {};
  EthernetFrame temp = out_frame.front();
  out_frame.pop();
  return temp;
}
