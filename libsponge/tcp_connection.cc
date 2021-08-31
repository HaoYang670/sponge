#include "tcp_connection.hh"

#include <iostream>

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { 
    return _sender.stream_in().buffer_size(); 
}

size_t TCPConnection::bytes_in_flight() const { 
    return _sender.bytes_in_flight(); 
}

size_t TCPConnection::unassembled_bytes() const { 
    return _receiver.unassembled_bytes(); 
}

size_t TCPConnection::time_since_last_segment_received() const { 
    return _time_since_last_receive; 
}

void TCPConnection::segment_received(const TCPSegment &seg) { 
    _time_since_last_receive = 0;
    
    const TCPHeader &header = seg.header();
    // reset sender and receiver and end the connection
    if(header.rst){
        _unclean_shutdown();
    }
    // deal with a normal segment
    else{
        _receiver.segment_received(seg);

        // update ackno and window size of sender
        if(header.ack){
            _sender.ack_received(header.ackno, header.win);
        }
        // passive open the sender
        if(header.syn){
            _sender.fill_window();
        }
        // passive closer, no need to linger
        if(header.fin && (!_sender.stream_in().eof())){
            _linger_after_streams_finish = false;
        }
        // if no seg in sender, create an empty one for ack
        if(seg.length_in_sequence_space() > 0 && _sender.segments_out().empty()){
            _sender.send_empty_segment();
        }
        _send_all_segments();
    }
}

bool TCPConnection::active() const { 
    if(_is_unclean_shutdown()){
        return false;
    }
    else if (!_receiver.stream_out().eof()){
        return true;
    }
    else if (!_sender.stream_in().eof() || _sender.bytes_in_flight() > 0){
        return true;
    }
    else if (_linger_after_streams_finish && _time_since_last_receive < 10 * _cfg.rt_timeout){
        return true;
    }
    return false;
}

size_t TCPConnection::write(const string &data) {
    const size_t written = _sender.stream_in().write(data);
    _sender.fill_window();
    _send_all_segments();
    return written;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    _time_since_last_receive += ms_since_last_tick;
    _sender.tick(ms_since_last_tick); 

    // reset both pairs if retransmit many times
    if(_sender.consecutive_retransmissions() > TCPConfig::MAX_RETX_ATTEMPTS){
        _send_rst();
        _unclean_shutdown();
    }
    else{
        _send_all_segments();
    }

}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input(); 
    _sender.fill_window();
    _send_all_segments();
}

void TCPConnection::connect() {
    // sender create the FIN segment
    _sender.fill_window();
    // send the FIN segment
    _send_all_segments();
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";

            // Your code here: need to send a RST segment to the peer
            _send_rst();
            _unclean_shutdown();
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}

/* ------------------------------private methods---------------------------------------- */

void TCPConnection::_send_all_segments(){
    while(!_sender.segments_out().empty()){
        TCPSegment seg = _sender.segments_out().front();
        // update the receiver's responsible fields
        seg.header().win = _receiver.window_size();
        if(_receiver.ackno().has_value()){
            seg.header().ack = true;
            seg.header().ackno = _receiver.ackno().value();
        }
        // send the segment
        _segments_out.push(seg);
        _sender.segments_out().pop();
    }
}

void TCPConnection::_unclean_shutdown(){
    _sender.stream_in().set_error();
    _receiver.stream_out().set_error();
}

bool TCPConnection::_is_unclean_shutdown() const{
    return _sender.stream_in().error() && _receiver.stream_out().error();
}

void TCPConnection::_send_rst() {
    if (_sender.segments_out().empty()){
        _sender.send_empty_segment();
    }
    _sender.segments_out().front().header().rst = true;
    _send_all_segments();
}
