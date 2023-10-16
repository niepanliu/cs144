#include "router.hh"

#include <iostream>
#include <limits>

using namespace std;

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length,
                        const optional<Address> next_hop,
                        const size_t interface_num )
{
  cerr << "DEBUG: adding route " << Address::from_ipv4_numeric( route_prefix ).ip() << "/"
       << static_cast<int>( prefix_length ) << " => " << ( next_hop.has_value() ? next_hop->ip() : "(direct)" )
       << " on interface " << interface_num << "\n";

  router_table.push_back({route_prefix, prefix_length, next_hop, interface_num});
}

void Router::route() 
{
  for(uint64_t i = 0; i < interfaces_.size(); i++) {
    std::optional<InternetDatagram> cur_dgam  = interfaces_[i].maybe_receive();
    if(cur_dgam.has_value()) {
      if(cur_dgam.value().header.ttl > 1) {
        cur_dgam.value().header.ttl--;
        cur_dgam.value().header.compute_checksum();
        auto it = max_prefix(cur_dgam.value().header.dst);
        if(it != router_table.end()){
          AsyncNetworkInterface& asy = interface(it->interface_num);
          asy.send_datagram(cur_dgam.value(), it->next_hop.value_or(Address::from_ipv4_numeric(cur_dgam.value().header.dst)));
        }
      }
    }
  }
}

std::vector<Router::Table>::iterator Router::max_prefix(uint32_t dst_ip)
{
  auto prefix_len = -1;
  auto res = router_table.end();
  for(auto it = router_table.begin(); it != router_table.end(); it++) {
    if(match_prefix(it->route_prefix, dst_ip, it->prefix_length) > prefix_len) {
      res = it;
      prefix_len = it->prefix_length;
    }
  }
  return res;
}

int Router::match_prefix(uint32_t src_ip, uint32_t dst_ip, uint8_t prefix_len)
{
  if(prefix_len == 0) 
    return 0;
  if(prefix_len > 32)
    return -1;
  uint8_t len = 32U - prefix_len;
  src_ip = src_ip >> len;
  dst_ip = dst_ip >> len;
  return src_ip == dst_ip ? prefix_len : -1;
}