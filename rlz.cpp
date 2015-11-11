#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <fstream>
#include <vector>
#include <stdint.h>
#include "rlz.hpp"

static bool __freqMapCmp(std::pair<int,double> a, std::pair<int,double> b) {
    return a.second < b.second;
}

int RLZCompressor::buildModel() {
    for(size_t i = 0; i<data.size(); i++) {
        addSymbolToModel(data[i]);
    }
}

void RLZCompressor::addSymbolToModel(int symbol) {
    freqsRaw[data[symbol] + 1]++;
    freqsRaw[0]++;

    //scale down on overflow
    if(freqsRaw[0] > code_max_freq) {
        for(int i = 0; i<freqsRaw.size(); i++) {
            if(freqsRaw[i]) {
                freqsRaw[i] /= 2;
                //do not allow zero for non-zero freqs
                if(!freqsRaw[i])
                    freqsRaw[i] = 1;
            }
        }
    }
}

void RLZCompressor::normModel() {
    freqsMap.clear();
    for(size_t i = 1; i<freqsRaw.size(); i++) {
        if(freqsRaw[i]>0) {
            freqsMap.push_back(Segment(i-1,freqsRaw[i]));
        }
    }
    std::sort(freqsMap.rbegin(), freqsMap.rend());

    for(int i = 0; i<freqsMap.size(); i++) {
        if(i) {
            freqsMap[i].low = freqsMap[i - 1].high;
        }
        freqsMap[i].high = freqsMap[i].low + freqsMap[i].length;
    }

    //normalize now
    double normFactor = freqsMap.rbegin()->high;
    for(int i = 0; i<freqsMap.size(); i++) {
        freqsMap[i].low /= normFactor;
        freqsMap[i].high /= normFactor;
        freqsMap[i].length /= normFactor;

        charToIdxMap[freqsMap[i].value] = i;
    }

}
#if 0
int RLZCompressor::compress() {
    double cHigh = 1.0f;
    double cLow = 0.0f;
    for(int i = 0; i<data.size(); i++) {
        double cLength = cHigh - cLow;
        int cVal = data[i];
        Segment nSegment = freqsMap[charToIdxMap[cVal]];
        cLow += nSegment.low * cLength;
        cHigh = cLow + nSegment.length * cLength;


        printf("cVal: %d cLow %f cHigh %f cLength %f\n", cVal, cLow, cHigh, cHigh - cLow);

        //normalize it!
        while(1) {
            if(cHigh < 0.5) {
                pushBit(0);
            } else if(cLow >= 0.5f) {
                pushBit(1);
                cLow -= 0.5f;
                cHigh -= 0.5f;
            } else if(cLow >= 0.25f && cHigh < 0.75f) {
                addBit();
                cLow -= 0.25f;
                cHigh -= 0.25f;
            } else {
                break;
            }
            cLow *= 2.0f;
            cHigh *= 2.0f;

        }

        printf("cVal: %d cLow %f cHigh %f cLength %f --norm\n", cVal, cLow, cHigh, cHigh - cLow);

    }
    addBit();
    if(cLow < 0.25f) {
        pushBit(0);
    } else {
        pushBit(1);
    }
    stopEmitter();
    return 1;
}


int RLZCompressor::decompress(size_t sz) {
    double cHigh = 1.0f;
    double cLow = 0.0f;
    int nBitsRead = 0;
    double value = 0;

    while(1) {
        int nBit = aquireBit();
        nBitsRead++;
        value += static_cast<double>(nBit) / static_cast<double>(nBitsRead + 1);

        for(int i = 0; i<freqsMap.size(); i++) {
            if(value >= freqsMap[i].low && value < freqsMap[i].high) {
                cLow = freqsMap[i].low;
                cHigh = freqsMap[i].high;
                result.push_back(freqsMap[i].value);
                printf("Decoded: %c\n", freqsMap[i].value);
                break;
            }
        }
    }

    while(result.size()<sz) {


        while(1) {
            if(cHigh < 0.5) {

            } else if(cLow >= 0.5f) {
                value -= 0.5f;
                cLow -= 0.5f;
                cHigh -= 0.5f;
                //nBitsRead--;
            } else if(cLow >= 0.25f && cHigh < 0.75f) {
                value -= 0.25f;
                cLow -= 0.25f;
                cHigh -= 0.25f;
                //nBitsRead--;
            } else {
                break;
            }
            cLow *= 2.0f;
            cHigh *= 2.0f;
            int nBit = aquireBit();
            //nBitsRead++;
            value = value * 2.0f + static_cast<double>(nBit) / static_cast<double>(nBitsRead + 1);
        }

        for(int i = 0; i<freqsMap.size(); i++) {
            if(value >= freqsMap[i].low && value < freqsMap[i].high) {
                cLow = freqsMap[i].low;
                cHigh = freqsMap[i].high;
                result.push_back(freqsMap[i].value);
                printf("Decoded: %c\n", freqsMap[i].value);

                break;
            }
        }


    }
    return 1;

}

#endif

int RLZCompressor::compress() {
    code_t cHigh = code_t_max;
    code_t cLow = 0;
    for(int i = 0; i<data.size(); i++) {
        code_t cLength = (cHigh - cLow) ;
        int cVal = data[i];
        Segment nSegment = freqsMap[charToIdxMap[cVal]];

        cLow = cLow + nSegment.low * cLength;
        cHigh = cLow + nSegment.length * cLength;

        //normalize it!
        while(1) {
            if(cHigh < code_h) {
                pushBit(0);
            } else if(cLow >= code_h) {
                pushBit(1);
                cLow -= code_h;
                cHigh -= code_h;
            } else if(cLow >= code_q && cHigh < code_3q) {
                addBit();
                cLow -= code_q;
                cHigh -= code_q;
            } else {
                break;
            }
            cLow *= 2;
            cHigh *= 2;

        }


    }
    addBit();
    if(cLow < code_q) {
        pushBit(0);
    } else {
        pushBit(1);
    }
    stopEmitter();
    return 1;
}


int RLZCompressor::decompress(size_t sz) {
    code_t cHigh = code_t_max;
    code_t cLow = 0;

    code_t value = 0;
    for(int i = 0; i<code_t_bits; i++) {
        value = 2*value + aquireBit();
    }
    printf("value: %d\n", value);

    while(result.size()<sz) {
        code_t cLength = cHigh - cLow;
        double cVal = static_cast<double>(value - cLow + 1) / static_cast<double>(cLength);
        printf("cVal %f\n", cVal);
        for(int i = 0; i<freqsMap.size(); i++) {
            if(cVal >= freqsMap[i].low && cVal < freqsMap[i].high) {
                result.push_back(freqsMap[i].value);

                Segment nSegment = freqsMap[i];

                cLow = cLow + nSegment.low * cLength;
                cHigh = cLow + nSegment.length * cLength;

                printf("Decoded: %c [%d] %f\n", freqsMap[i].value, i, cVal);
                break;
            }
        }

        while(1) {
            if(cHigh < code_h) {

            } else if(cLow >= code_h) {
                value -= code_h;
                cLow -= code_h;
                cHigh -= code_h;
            } else if(cLow >= code_q && cHigh < code_3q) {
                value -= code_q;
                cLow -= code_q;
                cHigh -= code_q;
            } else {
                break;
            }
            cLow *= 2;
            cHigh *= 2;
            int nBit=  aquireBit();
            value = value * 2 + nBit;
        }
    }

    return 1;

}


std::vector<char> RLZCompressor::packTable() {
    std::vector<char> result;
    printf("Packing: \n");
    for(int i = 0; i<256; i++) {
        printf("%d : ", i);
        for(int k = 3; k >=0; k--) {
            int val = (freqsRaw[i] >> (k*8)) & 0xff;
            result.push_back(val);
            printf(" %d ", val);
        }
        printf("\n");
    }
    return result;
}

void RLZCompressor::unpackTable(std::vector<char> &tPacked) {
    printf("Unpacking: \n");
    freqsRaw.clear();
    int tIdx = 0;
    for(int i = 0; i<256; i++) {
        uint32_t val = 0;
        for(int k = 3; k >=0; k--) {
            val |= (static_cast<unsigned char>(tPacked[tIdx]) << (k*8));
            tIdx++;
        }
        freqsRaw.push_back(val);
        printf("%d : %llu\n", i, val);
    }
}




