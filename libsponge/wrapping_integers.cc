#include "wrapping_integers.hh"
#include <iostream>
// Dummy implementation of a 32-bit wrapping integer

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! Transform an "absolute" 64-bit sequence number (zero-indexed) into a WrappingInt32
//! \param n The input absolute 64-bit sequence number
//! \param isn The initial sequence number
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) {
    //DUMMY_CODE(n, isn);
    const uint32_t last32Bits = n & 0xFFFFFFFF;
    return WrappingInt32{isn.raw_value() + last32Bits};
}

//! Transform a WrappingInt32 into an "absolute" 64-bit sequence number (zero-indexed)
//! \param n The relative sequence number
//! \param isn The initial sequence number
//! \param checkpoint A recent absolute 64-bit sequence number
//! \returns the 64-bit sequence number that wraps to `n` and is closest to `checkpoint`
//!
//! \note Each of the two streams of the TCP connection has its own ISN. One stream
//! runs from the local TCPSender to the remote TCPReceiver and has one ISN,
//! and the other stream runs from the remote TCPSender to the local TCPReceiver and
//! has a different ISN.
uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
    //DUMMY_CODE(n, isn, checkpoint);
    const uint64_t nVal = n.raw_value();
    const uint64_t isnVal = isn.raw_value();
    const uint64_t mask = 0xffffffff;
    const uint64_t width = 1ULL << 32;

    const uint64_t last32Bits = nVal >= isnVal ? nVal - isnVal : width - (isnVal - nVal);
    const uint64_t checkpointLast32 = checkpoint & mask;
    uint64_t first32Bits = checkpoint >> 32;

    if(last32Bits > checkpointLast32){
        if (first32Bits > 0 && last32Bits - checkpointLast32 > (width/2)){
            first32Bits -= 1;
        }
    }
    else if (last32Bits < checkpointLast32){
        if (first32Bits < mask && checkpointLast32 - last32Bits > (width/2)){
            first32Bits += 1;
        }
    }
    return (first32Bits<<32) | last32Bits;
}
