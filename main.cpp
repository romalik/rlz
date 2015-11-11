#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <fstream>
#include <vector>
#include "rlz.hpp"

enum {
    MODE_COMPRESS,
    MODE_DECOMPRESS
};

enum {
    ALG_AC,
    ALG_PPM
};

size_t loadFileToMem(std::string filename, std::vector<char> & dest) {
    size_t sz = 0;
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    sz = file.tellg();
    file.seekg(0, std::ios::beg);

    if(!sz) {
        return 0;
    }

    dest.clear();
    dest.resize(sz);

    if (!file.read(dest.data(), sz)) {
        return 0;
    } else {
        return sz;
    }
}

void writeToFile(std::string filename, std::vector<char> & src) {
    std::ofstream file(filename, std::ios::binary | std::ios::out);

    file.write(src.data(), src.size());
}

void badcommand(char ** argv) {
    printf("Usage: %s {c|d} input output [ppm]\n", argv[0]);
    exit(1);
}

int main(int argc, char ** argv) {

    if(argc < 4) {
        badcommand(argv);
    }

    if(!(argv[1][0] == 'c' || argv[1][0] == 'd')) {
        badcommand(argv);
    }

    int mode = MODE_COMPRESS;
    int alg = ALG_AC;

    if(argv[1][0] == 'c') {
        mode = MODE_COMPRESS;
    } else {
        mode = MODE_DECOMPRESS;
    }

    if(argc > 4) {
        if(!strcmp(argv[4], "ppm")) {
            alg = ALG_PPM;
        }
    }

    RLZCompressor compressor;

    size_t sz = loadFileToMem(argv[2], compressor.data);
    printf("File %s loaded, size %lu\n", argv[2], sz);

    if(sz) {
        compressor.buildModel();

        for(int i = 0; i<compressor.freqs.size(); i++) {
            printf("%d\t[%c]\t: %f\n", i, ((i>32)&&(i<127))?i:' ', compressor.freqs[i]);
        }

        printf("============= Sorted =============\n");
        for(int i = 0; i<compressor.freqsMap.size(); i++) {
            int v = compressor.freqsMap[i].value;
            double l = compressor.freqsMap[i].low;
            double h = compressor.freqsMap[i].high;
            printf("%d\t[%c]\t: %1.9f : %1.9f [%f]\n", v, ((v>32)&&(v<127))?v:' ', l, h, compressor.freqsMap[i].length);
        }
        printf("\n");
        compressor.compress();
        printf("\n");

        std::vector<char> output;
        output.push_back('R'); //magic
        output.push_back('L');
        output.push_back('Z');
        output.push_back('A'); //arithm
        output.push_back('A'); //adaptive

        //length
        for(int k = 3; k >=0; k--) {
            int val = (sz >> (k*8)) & 0xff;
            output.push_back(val);
        }


        std::vector<char> table = compressor.packTable();
        output.insert(output.end(), table.begin(), table.end());
        output.insert(output.end(), compressor.result.begin(), compressor.result.end());

        writeToFile(argv[3], output);


    }
    return 0;
}

