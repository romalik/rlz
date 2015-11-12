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
        addSymbolToModel(static_cast<unsigned char>(data[i]));
    }
}

void RLZCompressor::addSymbolToModel(int symbol) {

    if(freqsRaw[0] < code_max_freq) {
        freqsRaw[symbol + 1]++;
        freqsRaw[0]++;
    } else {
        return;
    }
/*
    //scale down on overflow
    if(freqsRaw[0] > code_max_freq) {
        for(int i = 0; i<freqsRaw.size(); i++) {
            //do not allow zero for non-zero freqs
            if(freqsRaw[i]>1) {
                freqsRaw[i] /= 2;
            }
        }
    }
    */
}


void RLZCompressor::requestAddSymbolToModel(int symbol) {
    addQueue.push_back(symbol);

    if(addQueue.size() >= adaptiveBlockSize) {
        for(int i = 0; i<addQueue.size(); i++) {
            addSymbolToModel(addQueue[i]);
        }
        addQueue.clear();
        normModel();
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
        } else {
            freqsMap[i].low = 0;
        }
        freqsMap[i].high = freqsMap[i].low + freqsMap[i].length;
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
    for(size_t i = 0; i<data.size(); i++) {
        code_t cLength = (cHigh - cLow) + 1;
        int cVal = static_cast<unsigned char>(data[i]);
        //printf("CVal: %d\n", cVal);

        //printf("tIdx: %d\n", charToIdxMap[cVal]);
        Segment nSegment = freqsMap[charToIdxMap[cVal]];

        cHigh = cLow + (nSegment.high * cLength) / freqsRaw[0] - 1;
        cLow = cLow + (nSegment.low * cLength) / freqsRaw[0];

        if(compMode == COMP_MODEL_ADAPTIVE) {
            requestAddSymbolToModel(cVal);
        }

        //normalize it!
        while(1) {
            if(cHigh < code_h) {
                pushBit(0);
            } else if(cLow >= code_h) {
                pushBit(1);
            } else if(cLow >= code_q && cHigh < code_3q) {
                addBit();
                cLow -= code_q;
                cHigh -= code_q;
            } else {
                break;
            }
            cHigh <<= 1;
            cHigh++;
            cLow <<= 1;
            cHigh &= code_t_max;
            cLow &= code_t_max;
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
        value <<= 1;
        value += aquireBit();
    }
    //printf("value: %llu\n", value);

    while(result.size()<sz) {
        code_t cLength = static_cast<code_t>(cHigh - cLow) + 1;
        code_t cVal = ((value - cLow + 1) * freqsRaw[0] - 1) / cLength;
        //printf("cVal %llu value %llu cLow %llu cHigh %llu\n", cVal, value, cLow, cHigh);
        for(int i = 0; i<freqsMap.size(); i++) {
            //printf("cVal %d vs low %d vs high %d len %d total %d i %d\n", cVal, freqsMap[i].low, freqsMap[i].high, freqsMap[i].length, freqsRaw[0], i);

            if(cVal >= freqsMap[i].low && cVal < freqsMap[i].high) {

                result.push_back(freqsMap[i].value);

                Segment nSegment = freqsMap[i];

                cHigh = cLow + (nSegment.high * cLength) / freqsRaw[0] - 1;
                cLow = cLow + (nSegment.low * cLength) / freqsRaw[0];

                if(compMode == COMP_MODEL_ADAPTIVE) {
                    requestAddSymbolToModel(freqsMap[i].value);
                }

                //printf("Decoded: %c [%d]\n", freqsMap[i].value, i);
                break;
            }
        }

        if(result.size() >= sz)
            break;

        while(1) {
            //printf("value %llu cLow %llu cHigh %llu\n", value, cLow, cHigh);
            if(cHigh < code_h) {
                //skip
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
            cLow <<= 1;
            cHigh <<= 1;
            cHigh++;
            value <<= 1;
            int nBit = aquireBit();
            //printf("nBit %d\n", nBit);
            value += nBit;

        }
    }
    printf("End!! Sz: %d\n", sz);
    return 1;

}


std::vector<char> RLZCompressor::packTable() {
    std::vector<char> result;
    //printf("Packing: \n");
    for(int i = 0; i<257; i++) {
        //printf("%d : ", i);
        for(int k = 3; k >=0; k--) {
            int val = (freqsRaw[i] >> (k*8)) & 0xff;
            result.push_back(val);
            //printf(" %d ", val);
        }
        //printf("\n");
    }
    return result;
}

void RLZCompressor::unpackTable(std::vector<char> &tPacked) {
    //printf("Unpacking: \n");
    freqsRaw.clear();
    int tIdx = 0;
    for(int i = 0; i<257; i++) {
        uint32_t val = 0;
        for(int k = 3; k >=0; k--) {
            val |= (static_cast<unsigned char>(tPacked[tIdx]) << (k*8));
            tIdx++;
        }
        freqsRaw.push_back(val);
        //printf("%d : %llu\n", i, val);
    }
}




