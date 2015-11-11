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
    size_t maxCnt = 0;
    for(size_t i = 0; i<data.size(); i++) {
        freqsRaw[data[i]]++;
        if(freqsRaw[data[i]] > maxCnt) {
            maxCnt = freqsRaw[data[i]];
        }
    }
    freqsMap.clear();
    if(maxCnt) {
        for(size_t i = 0; i<freqs.size(); i++) {
            freqs[i] = static_cast<double>(freqsRaw[i]) / static_cast<double>(maxCnt);
        }

        for(size_t i = 0; i<freqs.size(); i++) {
            if(freqs[i]>0) {
                freqsMap.push_back(Segment(i,freqs[i]));
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

        return 1;
    } else {
        return 0;
    }
}

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


std::vector<char> RLZCompressor::packTable() {
    std::vector<char> result;
    for(int i = 0; i<256; i++) {
        for(int k = 3; k >=0; k--) {
            int val = (freqsRaw[i] >> (k*8)) & 0xff;
            result.push_back(val);
        }
    }
    return result;
}




