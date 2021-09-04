#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

#include <iostream>
#include <set>

// Translates from {IP datagram, next hop address} to link-layer frame, and from link-layer frame to IP datagram

// For Lab 5, please replace with a real implementation that passes the
// automated checks run by `make check_lab5`.

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface(const EthernetAddress &ethernet_address, const Address &ip_address)
    : _ethernet_address(ethernet_address), _ip_address(ip_address) {
    cerr << "DEBUG: Network interface has Ethernet address " << to_string(_ethernet_address) << " and IP address "
         << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but may also be another host if directly connected to the same network as the destination)
//! (Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) with the Address::ipv4_numeric() method.)
void NetworkInterface::send_datagram(const InternetDatagram &dgram, const Address &next_hop) {
    // convert IP address of next hop to raw 32-bit representation (used in ARP header)
    const uint32_t next_hop_ip = next_hop.ipv4_numeric();
    // the cache store the mac addr
    if(_arp_cache.count(next_hop_ip) > 0){
        _frames_out.push(_create_ipv4_frame(dgram, next_hop_ip));
    }
    else{
        // put the datagram in writing cache for later sending
        _put_in_waiting(dgram, next_hop_ip);
        // send an arp request
        if (_request_cache.count(next_hop_ip) == 0){
            const ARPMessage &request = _createARP(ARPMessage::OPCODE_REQUEST, {}, next_hop_ip);
            _frames_out.push(_create_arp_frame(request, ETHERNET_BROADCAST));
            _request_cache[next_hop_ip] = _current_time;
        }
    }
}

//! \param[in] frame the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame(const EthernetFrame &frame) {
    const EthernetAddress dst = frame.header().dst;

    if(dst == _ethernet_address || dst == ETHERNET_BROADCAST){
        // an ipv4 datagram in this frame, parse it
        if(frame.header().type == EthernetHeader::TYPE_IPv4) {
            InternetDatagram result;
            if(result.parse(frame.payload()) == ParseResult::NoError){
                return result;
            }
        }
        // an arp datagram in this frame
        else {
            ARPMessage result;

            if(result.parse(frame.payload()) == ParseResult::NoError){
                // update arp cache and send waiting datagrams
                _arp_cache[result.sender_ip_address] = std::make_pair(result.sender_ethernet_address, _current_time);
                // reply the arp request
                if(result.opcode == ARPMessage::OPCODE_REQUEST && result.target_ip_address == _ip_address.ipv4_numeric()){
                    const ARPMessage &reply = _createARP(ARPMessage::OPCODE_REPLY, result.sender_ethernet_address, result.sender_ip_address);
                    _frames_out.push(_create_arp_frame(reply, result.sender_ethernet_address));
                }
                _send_waiting_datagrams(result.sender_ip_address);
            }
        }
    }
    return {};
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick(const size_t ms_since_last_tick) { 
    _current_time += ms_since_last_tick;
    _clean_arp_cache(30*1000);
    _clean_request_cache(5*1000);
}

/*------------------------- private methods -------------------------------------*/
void NetworkInterface::_clean_arp_cache(const size_t threshold){
    std::set<uint32_t> removed;
    for(auto const &item : _arp_cache){
        if(_current_time - item.second.second > threshold){
            removed.insert(item.first);
        }
    }
    for(uint32_t key : removed){
        _arp_cache.erase(key);
    }
}

void NetworkInterface::_put_in_waiting(const InternetDatagram &dgram, const uint32_t ip_addr){
    if (_waiting_datagrams.count(ip_addr) == 0){
        _waiting_datagrams[ip_addr] = {};
    }
    _waiting_datagrams[ip_addr].push(dgram);
}

void NetworkInterface::_send_waiting_datagrams(const uint32_t ip_addr){
    if(_waiting_datagrams.count(ip_addr) > 0){
        queue<InternetDatagram> &datagrams = _waiting_datagrams[ip_addr];
        while(!datagrams.empty()){
            _frames_out.push(_create_ipv4_frame(datagrams.front(), ip_addr));
            datagrams.pop();
        }
        _waiting_datagrams.erase(ip_addr);
    }
}

const EthernetFrame NetworkInterface::_create_ipv4_frame(const InternetDatagram &dgram, const uint32_t target_ip){
    const EthernetAddress &mac = _arp_cache[target_ip].first;
    EthernetFrame frame;
    frame.header() = EthernetHeader{mac, _ethernet_address, EthernetHeader::TYPE_IPv4};
    frame.payload() = dgram.serialize();
    return frame;
}

const EthernetFrame NetworkInterface::_create_arp_frame(const ARPMessage &arp_msg, const EthernetAddress target_mac){
    EthernetFrame frame;
    frame.header() = EthernetHeader{target_mac, _ethernet_address, EthernetHeader::TYPE_ARP};
    frame.payload() = arp_msg.serialize();
    return frame;
}

void NetworkInterface::_clean_request_cache(const size_t threshold){
    set<uint32_t> removed;
    for(auto const &item : _request_cache){
        if(_current_time - item.second > threshold){
            removed.insert(item.first);
        }
    }
    for(uint32_t ip_addr : removed){
        _request_cache.erase(ip_addr);
    }
}

const ARPMessage NetworkInterface::_createARP(const uint16_t opcode, const EthernetAddress target_ethernet_addr, const uint32_t target_ip_addr){
    ARPMessage msg;
    msg.opcode = opcode;
    msg.sender_ip_address = _ip_address.ipv4_numeric();
    msg.sender_ethernet_address = _ethernet_address;
    msg.target_ip_address = target_ip_addr;
    msg.target_ethernet_address = target_ethernet_addr;
    return msg;
}
