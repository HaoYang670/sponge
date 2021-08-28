#include "tcp_sender.hh"
#include "tcp_config.hh"

#include <random>
#include <algorithm>

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

using namespace std;

Timer::Timer(const size_t rto)
    : _init_rto{rto}
    , _current_rto{rto} {}

void Timer::increment(const size_t ms){
    _current_time += ms;
}

bool Timer::has_expired() const{
    return _current_time >= _current_rto;
}

void Timer::double_rto(){
    _current_rto *= 2;
    _consecutive_doubles += 1;
}

void Timer::reset_time(){
    _current_time = 0;
}

void Timer::reset_rto(){
    _current_rto = _init_rto;
    _consecutive_doubles = 0;
}

size_t Timer::consecutive_doubles() const{
    return _consecutive_doubles;
}

bool Timer::can_back_off() const{
    return _back_off;
}

void Timer::set_back_off(bool can) {
    _back_off = can;
}




//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _stream(capacity) 
    , _timer(retx_timeout){}

uint64_t TCPSender::bytes_in_flight() const { 
    return _next_seqno - _ackno;
}

void TCPSender::fill_window() {
    if(_next_seqno == 0){ // open TCP 
        TCPSegment seg;
        seg.header().seqno = wrap(_next_seqno, _isn);
        seg.header().syn = true;
        _segments_out.push(seg);
        _flying_segments.push(seg);
        _next_seqno += seg.length_in_sequence_space();
    }


    // fill new space as much as possible
    while(_next_seqno <= _can_send_seqno && (!_stream.buffer_empty())){
        TCPSegment seg;
        seg.header().seqno = wrap(_next_seqno, _isn);

        const size_t new_space = _can_send_seqno - _next_seqno + 1;
        const size_t len = std::min(new_space, TCPConfig::MAX_PAYLOAD_SIZE);
        seg.payload() = _stream.read(len);

        if(_stream.eof() && (_next_seqno + seg.length_in_sequence_space() <= _can_send_seqno)){
            seg.header().fin = true;
            _send_FIN = true;
        }

        _next_seqno += seg.length_in_sequence_space();
        _segments_out.push(seg);
        _flying_segments.push(seg);
    }
    
    // reach eof but not send
    if(!_send_FIN && _stream.eof() && _next_seqno <= _can_send_seqno){
        TCPSegment seg;
        seg.header().seqno = wrap(_next_seqno, _isn);
        seg.header().fin = true;

        _segments_out.push(seg);
        _flying_segments.push(seg);
        _next_seqno += seg.length_in_sequence_space();
        _send_FIN = true;
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) { 
    const uint64_t new_ackno = unwrap(ackno, _isn, _next_seqno);

    // this is an old ack
    if(new_ackno < _ackno){
        return;
    }

    // update timer
    if(new_ackno > _ackno){
        _timer.reset_time();
        _timer.reset_rto();
        _timer.set_back_off(window_size > 0 ? true : false);
    }

    // update ackno and new_space
    _ackno = new_ackno;
    _can_send_seqno = max(max(_ackno + window_size - 1, _ackno), _can_send_seqno);

    
    // remove acked segments from flying set
    while (! _flying_segments.empty()){
        const TCPSegment oldest = _flying_segments.front();
        const size_t last_byte = unwrap(oldest.header().seqno, _isn, _ackno) + oldest.length_in_sequence_space() - 1;

        if (last_byte < _ackno){
            _flying_segments.pop();
        }
        else{
            break;
        }
    }
    fill_window();
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) { 
    _timer.increment(ms_since_last_tick);
    if(_timer.has_expired()){
        if(!_flying_segments.empty()){
            _segments_out.push(_flying_segments.front());
            if(_timer.can_back_off()){
                _timer.double_rto();
            }
        }
        _timer.reset_time();
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { 
    return _timer.consecutive_doubles(); 
}

void TCPSender::send_empty_segment() {
    TCPSegment seg;
    seg.header().seqno = wrap(_next_seqno, _isn);
    seg.header().ack = true;
    _segments_out.push(seg);
}
