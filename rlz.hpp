#ifndef RLZ_HPP_
#define RLZ_HPP_

#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <fstream>
#include <vector>
#include <deque>
#include <map>
#include <stdint.h>

struct Segment {
    int value;
    double low;
    double high;
    double length;
    Segment() : value(0), length(0), low(0), high(0) {}
    Segment(int _v) : value(_v), length(0), low(0), high(0) {}
    Segment(int _v, double _len) : value(_v), length(_len), low(0), high(0) {}
    bool operator<(const Segment & that) const {
        return this->length < that.length;
    }
};

class RLZCompressor {
public:
    RLZCompressor() {
        freqs.resize(256,0);
        charToIdxMap.resize(256,-1);
        freqsRaw.resize(256,0);
        extBits = 0;
        cByte = 0;
        cBitPtr = 7;
    }
    std::vector<char> data;
    std::vector<double> freqs;
    std::vector<Segment> freqsMap;
    std::vector<int> charToIdxMap;
    std::vector<uint32_t> freqsRaw;


    int buildModel();
    int compress();

    unsigned char cByte;
    int cBitPtr;
    int extBits;
    std::vector<char> result;
    void emitBit(int bit) {
        printf("BIT %d\n", bit?1:0);
        if(bit) {
            cByte |= (1<<cBitPtr);
        } else {
            cByte &= ~(1<<cBitPtr);
        }
        cBitPtr--;
        if(cBitPtr < 0) {
            cBitPtr = 7;
            printf("Byte 0x%02X\n", cByte);
            result.push_back(cByte);
            cByte = 0;
        }
    }

    void stopEmitter() {
        if(cBitPtr < 7) {
            result.push_back(cByte);
            printf("Final Byte 0x%02X\n", cByte);
            cByte = 0;
            cBitPtr = 7;
        }
    }



    void pushBit(int bit) {
        emitBit(bit);
        while(extBits) {
            emitBit(!bit);
            extBits--;
        }
    }

    void addBit() {
        extBits++;
    }

    std::vector<char> packTable();
};




#endif
