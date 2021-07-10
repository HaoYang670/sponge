#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

//template <typename... Targs>
//void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

ByteStream::ByteStream(const size_t capacity):
    //DUMMY_CODE(capacity); 
    maxCapacity (capacity),
    bytes({}),
    readBytes(0),
    writtenBytes(0),
    inputEnd(false) {}
    


size_t ByteStream::write(const string &data) {
    //DUMMY_CODE(data);
    const size_t dataSize = min(remaining_capacity(), data.size());
    // add new string to the tail of old string
    bytes = bytes + data.substr(0, dataSize);
    writtenBytes += dataSize;
    return dataSize;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    //DUMMY_CODE(len);
    const size_t dataSize = min(buffer_size(), len);
    return bytes.substr(0, dataSize);
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) { 
    //DUMMY_CODE(len);
    const size_t dataSize = min(buffer_size(), len);
    bytes = bytes.erase(0, dataSize);
    readBytes += dataSize;
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    //DUMMY_CODE(len);
    const size_t dataSize = min(buffer_size(), len);
    const string output = peek_output(dataSize);

    pop_output(dataSize);
    return output;
}

void ByteStream::end_input() {
    inputEnd = true;
}

bool ByteStream::input_ended() const { 
    return inputEnd; 
}

size_t ByteStream::buffer_size() const { 
    return bytes.size(); 
}

bool ByteStream::buffer_empty() const { 
    return buffer_size() == 0;
}

bool ByteStream::eof() const {
    return buffer_empty() && inputEnd;
}

size_t ByteStream::bytes_written() const {
    return writtenBytes;
}

size_t ByteStream::bytes_read() const {
    return readBytes;
}

size_t ByteStream::remaining_capacity() const {
    return maxCapacity - buffer_size();
}
