#ifndef SPONGE_LIBSPONGE_NETWORK_INTERFACE_HH
#define SPONGE_LIBSPONGE_NETWORK_INTERFACE_HH

#include "ethernet_frame.hh"
#include "tcp_over_ip.hh"
#include "tun.hh"

#include <optional>
#include <queue>
#include <map>

//! \brief A "network interface" that connects IP (the internet layer, or network layer)
//! with Ethernet (the network access layer, or link layer).

//! This module is the lowest layer of a TCP/IP stack
//! (connecting IP with the lower-layer network protocol,
//! e.g. Ethernet). But the same module is also used repeatedly
//! as part of a router: a router generally has many network
//! interfaces, and the router's job is to route Internet datagrams
//! between the different interfaces.

//! The network interface translates datagrams (coming from the
//! "customer," e.g. a TCP/IP stack or router) into Ethernet
//! frames. To fill in the Ethernet destination address, it looks up
//! the Ethernet address of the next IP hop of each datagram, making
//! requests with the [Address Resolution Protocol](\ref rfc::rfc826).
//! In the opposite direction, the network interface accepts Ethernet
//! frames, checks if they are intended for it, and if so, processes
//! the the payload depending on its type. If it's an IPv4 datagram,
//! the network interface passes it up the stack. If it's an ARP
//! request or reply, the network interface processes the frame
//! and learns or replies as necessary.
class NetworkInterface {
  private:
    //! Ethernet (known as hardware, network-access-layer, or link-layer) address of the interface
    EthernetAddress _ethernet_address;

    //! IP (known as internet-layer or network-layer) address of the interface
    Address _ip_address;

    //! outbound queue of Ethernet frames that the NetworkInterface wants sent
    std::queue<EthernetFrame> _frames_out{};

    //! the map from ipv4 to (MAC address, time stamp) in the cache 
    //! id address => mac
    std::map<uint32_t, std::pair<EthernetAddress, size_t>> _arp_cache{};

    //! datagrams that wait to be sent
    //! ip address => datagrams
    std::map<uint32_t, std::queue<InternetDatagram>> _waiting_datagrams{};

    //! arp request sent in last several seconds 
    //! ip address => time stemp
    std::map<uint32_t, uint32_t> _request_cache{};

    //! current time in ms
    size_t _current_time {};

    //! remove items in arp cache that are out of date
    void _clean_arp_cache(const size_t threshold);

    //! put the datagram in the waiting list
    void _put_in_waiting(const InternetDatagram &dgram, const uint32_t ip_addr);

    //! send out the waiting datagrams
    void _send_waiting_datagrams(const uint32_t ip_addr);

    //! remove out of date requests
    void _clean_request_cache(const size_t threshold);

    //! create a specific ARP message
    const struct ARPMessage _createARP(const uint16_t opcode, const EthernetAddress target_ethernet_addr, const uint32_t target_ip_addr);

    //! create a frame whose payload is an ipv4 datagram
    const EthernetFrame _create_ipv4_frame(const InternetDatagram &dgram, const uint32_t target_ip);

    //! create a frame whose pay load is an arp message
    const EthernetFrame _create_arp_frame(const struct ARPMessage &arp_msg, const EthernetAddress target_mac);

  public:
    //! \brief Construct a network interface with given Ethernet (network-access-layer) and IP (internet-layer) addresses
    NetworkInterface(const EthernetAddress &ethernet_address, const Address &ip_address);

    //! \brief Access queue of Ethernet frames awaiting transmission
    std::queue<EthernetFrame> &frames_out() { return _frames_out; }

    //! \brief Sends an IPv4 datagram, encapsulated in an Ethernet frame (if it knows the Ethernet destination address).

    //! Will need to use [ARP](\ref rfc::rfc826) to look up the Ethernet destination address for the next hop
    //! ("Sending" is accomplished by pushing the frame onto the frames_out queue.)
    void send_datagram(const InternetDatagram &dgram, const Address &next_hop);

    //! \brief Receives an Ethernet frame and responds appropriately.

    //! If type is IPv4, returns the datagram.
    //! If type is ARP request, learn a mapping from the "sender" fields, and send an ARP reply.
    //! If type is ARP reply, learn a mapping from the "sender" fields.
    std::optional<InternetDatagram> recv_frame(const EthernetFrame &frame);

    //! \brief Called periodically when time elapses
    void tick(const size_t ms_since_last_tick);
};

#endif  // SPONGE_LIBSPONGE_NETWORK_INTERFACE_HH
