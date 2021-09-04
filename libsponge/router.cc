#include "router.hh"

#include <iostream>

using namespace std;


// Given an incoming Internet datagram, the router decides
// (1) which interface to send it out on, and
// (2) what next hop address to send it to.

// For Lab 6, please replace with a real implementation that passes the
// automated checks run by `make check_lab6`.


template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

//! \param[in] route_prefix The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
//! \param[in] prefix_length For this route to be applicable, how many high-order (most-significant) bits of the route_prefix will need to match the corresponding bits of the datagram's destination address?
//! \param[in] next_hop The IP address of the next hop. Will be empty if the network is directly attached to the router (in which case, the next hop address should be the datagram's final destination).
//! \param[in] interface_num The index of the interface to send the datagram out on.
void Router::add_route(const uint32_t route_prefix,
                       const uint8_t prefix_length,
                       const optional<Address> next_hop,
                       const size_t interface_num) {
    cerr << "DEBUG: adding route " << Address::from_ipv4_numeric(route_prefix).ip() << "/" << int(prefix_length)
         << " => " << (next_hop.has_value() ? next_hop->ip() : "(direct)") << " on interface " << interface_num << "\n";

    // Your code here.
    _forwarding_table.push_back({route_prefix, prefix_length, next_hop, interface_num});
}

//! \param[in] dgram The datagram to be routed
void Router::route_one_datagram(InternetDatagram &dgram) {
    // Your code here.
    if(dgram.header().ttl <= 1){
        return;
    }
    const uint32_t dst = dgram.header().dst;
    uint8_t longest = 0;
    std::optional<size_t> interface_num {};
    std::optional<Address> next_hop {};

    for(auto &rcd : _forwarding_table){
        const uint8_t shift = 32 - rcd.prefix_length;
        if(shift == 32){
            if(!interface_num.has_value()){
                interface_num = {rcd.interface_num};
                next_hop = rcd.next_hop;
            }
        }
        else if(dst >> shift == rcd.route_prefix >> shift && longest < rcd.prefix_length){
            interface_num = {rcd.interface_num};
            next_hop = rcd.next_hop;
            longest = rcd.prefix_length;
        }
    }

    if(interface_num.has_value()){
        if(!next_hop.has_value()){
            next_hop = {Address::from_ipv4_numeric(dst)};
        }
        dgram.header().ttl --;
        interface(interface_num.value()).send_datagram(dgram, next_hop.value());
    }
}

void Router::route() {
    // Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
    for (auto &interface : _interfaces) {
        auto &queue = interface.datagrams_out();
        while (not queue.empty()) {
            route_one_datagram(queue.front());
            queue.pop();
        }
    }
}