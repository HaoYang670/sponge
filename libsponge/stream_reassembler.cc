#include "stream_reassembler.hh"
#include <iostream>
// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

//template <typename... Targs>
//void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : 
    _output(capacity), 
    _capacity(capacity),
    unassembledBytes(0),
    remarkEOF(false),
    endIndex(0),
    subStrings() {}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    //DUMMY_CODE(data, index, eof);
    if(eof){
        remarkEOF = true;
        endIndex = index + data.size();
    }
    Segment next{data, index};
    mergeSegment(next);


    writeSegments();

    if(remarkEOF && _output.bytes_written() == endIndex){
        _output.end_input();
    }
}

size_t StreamReassembler::unassembled_bytes() const { 
    return unassembledBytes; 
}

bool StreamReassembler::empty() const { 
    return subStrings.empty(); 
}

void StreamReassembler::mergeSegment(const Segment & seg){
    const size_t headIndex = _output.bytes_written();
    size_t start = max(seg.index, headIndex);
    size_t end = min(seg.index + seg.data.size(), headIndex+_output.remaining_capacity());
    if(start >= end) return;

    std::set<Segment> removedSegs;

    for(Segment oldSeg : subStrings){
        const size_t oldStart =  oldSeg.index;
        const size_t oldEnd = oldStart + oldSeg.data.size();

        if(oldStart <= start){
            // no overlap between the two segment
            if(oldEnd <= start) continue;
            // data between [start, oldEnd] is overlap
            else if (oldEnd <= end) start = oldEnd;
            // old segment fully cover the new segment
            else return;
        }
        else if (oldStart < end){
            // new segment fully cover the old segment
            if(oldEnd <= end) removedSegs.insert(oldSeg);
            // overlap between [oldStart, end]
            else end = oldStart;
        }
        // no overlap between the two segment
        else break;
    }

    for(auto oldSeg : removedSegs){
        subStrings.erase(oldSeg);
        unassembledBytes -= oldSeg.data.size();
    }

    if(end - start > 0){
        const Segment newSeg{seg.data.substr(start - seg.index, end - start), start};
        subStrings.insert(newSeg);
        unassembledBytes += (end - start);
    }
}

void StreamReassembler::writeSegments(){
    // write sub strings to output
    while(!subStrings.empty()){
        auto next = subStrings.begin();        
        if((*next).index == _output.bytes_written()){
            subStrings.erase(next);
            unassembledBytes -= _output.write((*next).data);
        }
        else break;
    }
}
