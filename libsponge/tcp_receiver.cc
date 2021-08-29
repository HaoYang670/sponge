#include "tcp_receiver.hh"
#include <iostream>

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    if(_reassembler.stream_out().input_ended()){
        return;
    }

    const TCPHeader header = seg.header();

    if(!_hasSYN && header.syn) {
        _hasSYN = true;
        _isn = header.seqno;
    }
    if(_hasSYN) {
        // whether a fin flag has been received
        _hasFIN = _hasFIN || header.fin;

        const uint64_t absIdx = unwrap(header.seqno, _isn, _reassembler.stream_out().bytes_written() + 1);
        // a segment with no syn flag and absIdx == 0 is invalid
        if(absIdx > 0 || header.syn){
            const uint64_t streamIdx = absIdx > 0 ? absIdx-1 : 0;
            const string content = seg.payload().copy();
            _reassembler.push_substring(content, streamIdx, header.fin);
        }
    }
}

optional<WrappingInt32> TCPReceiver::ackno() const { 
    if (!_hasSYN) return {};
    else {
        uint ackIdx = _reassembler.stream_out().bytes_written() + 1;  // plus SYN flag
        if(_reassembler.stream_out().input_ended()) ackIdx ++; // plus FIN flag
        return {wrap(ackIdx, _isn)};
    }
}

size_t TCPReceiver::window_size() const { 
    return _reassembler.stream_out().remaining_capacity(); 
}
