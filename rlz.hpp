#ifndef RLZ_HPP_
#define RLZ_HPP_

#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <fstream>
#include <vector>
#include <deque>
#include <map>
//#include <stdint.h>
#include <algorithm>
#include <limits.h>


class RLZCompressor {

public:
    typedef unsigned long long code_t;

    static const code_t code_t_base_bits = 32;

    static const code_t code_t_bits = (code_t_base_bits + 3) / 2;
    static const code_t code_t_freq_bits = (code_t_base_bits - code_t_bits);
    static const code_t code_t_max = (1ULL << (code_t_bits)) - 1;
    static const code_t code_q = (1ULL << (code_t_bits - 2));
    static const code_t code_h = 2*code_q;
    static const code_t code_3q = 3*code_q;
    static const code_t code_max_freq = (1ULL << (code_t_freq_bits)) - 1;//(code_t_max >> 2);


    struct Segment {
        int value;
        code_t low;
        code_t high;
        code_t length;
        Segment() : value(0), length(0), low(0), high(0) {}
        Segment(int _v) : value(_v), length(0), low(0), high(0) {}
        Segment(int _v, code_t _len) : value(_v), length(_len), low(0), high(0) {}
        bool operator<(const Segment & that) const {
            return this->length < that.length;
        }
    };


    enum {
        COMP_MODEL_FIXED,
        COMP_MODEL_ADAPTIVE,
        COMP_MODEL_CUSTOM
    };



    RLZCompressor() {
        charToIdxMap.resize(256,-1);
        freqsRaw.resize(257,1);
        freqsRaw[0] = 256;
        extBits = 0;
        eByte = 0;
        eBitPtr = 7;

        aByteIdx = 0;
        aBitPtr = 7;

        compMode = COMP_MODEL_CUSTOM;
        adaptiveBlockSize = 32;
    }
    std::vector<char> data;
    std::vector<Segment> freqsMap;
    std::vector<int> charToIdxMap;
    std::vector<code_t> freqsRaw;

    std::vector<char> result;

    void addSymbolToModel(int symbol);

    std::vector<int> addQueue;
    void requestAddSymbolToModel(int symbol);

    int compMode;
    int adaptiveBlockSize;


    int buildModel();
    void normModel();
    int compress();
    int decompress(size_t sz);


    //aquirer
    int aByteIdx;
    int aBitPtr;
    int aquireBit() {
        if(aByteIdx < data.size()) {
            int val = data[aByteIdx] & (1 << aBitPtr);
            aBitPtr--;
            if(aBitPtr < 0) {
                aBitPtr = 7;
                aByteIdx++;
            }
            //printf("Bit : %d\n", val);
            return (val>0)?1:0;
        } else {
            return -1;
        }
    }

    //emitter
    unsigned char eByte;
    int eBitPtr;
    int extBits;
    void emitBit(int bit) {
        //printf("BIT %d\n", bit?1:0);
        if(bit) {
            eByte |= (1<<eBitPtr);
        } else {
            eByte &= ~(1<<eBitPtr);
        }
        eBitPtr--;
        if(eBitPtr < 0) {
            eBitPtr = 7;
            //printf("Byte 0x%02X\n", eByte);
            result.push_back(eByte);
            eByte = 0;
        }
    }

    void stopEmitter() {
        //if(eBitPtr < 7) {
            result.push_back(eByte);
            //printf("Final Byte 0x%02X\n", eByte);
            eByte = 0;
            eBitPtr = 7;
        //}
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
    void unpackTable(std::vector<char> & tPacked);
};




#endif
