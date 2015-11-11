#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <fstream>
#include <vector>
#include "rlz.hpp"
#include <string.h>
enum {
    MODE_COMPRESS,
    MODE_DECOMPRESS,
    MODE_GENERATE_MODEL
};

enum {
    ALG_AC,
    ALG_PPM
};

size_t loadFileToMem(std::string filename, std::vector<char> & dest) {
    size_t sz = 0;
    std::ifstream file(filename.c_str(), std::ios::binary | std::ios::ate);
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
    std::ofstream file(filename.c_str(), std::ios::binary | std::ios::out);

    file.write(src.data(), src.size());
}

void badcommand(char ** argv) {
    printf("Usage: %s {c|d|g} input output [ppm]\n", argv[0]);
    exit(1);
}

int main(int argc, char ** argv) {

    if(argc < 4) {
        badcommand(argv);
    }

    if(!(argv[1][0] == 'c' || argv[1][0] == 'd' || argv[1][0] == 'g')) {
        badcommand(argv);
    }

    int mode = MODE_COMPRESS;
    int alg = ALG_AC;

    if(argv[1][0] == 'c') {
        mode = MODE_COMPRESS;
    } else if(argv[1][0] == 'd') {
        mode = MODE_DECOMPRESS;
    } else {
        mode = MODE_GENERATE_MODEL;
    }

    if(argc > 4) {
        if(!strcmp(argv[4], "ppm")) {
            alg = ALG_PPM;
        }
    }


    if(mode == MODE_COMPRESS) {
        RLZCompressor compressor;

        compressor.compMode = RLZCompressor::COMP_MODEL_CUSTOM;

        size_t sz = loadFileToMem(argv[2], compressor.data);
        printf("File %s loaded, size %lu\n", argv[2], sz);

        if(sz) {
            compressor.buildModel();
            compressor.normModel();

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


            /* File Format
             * 0: R     //magic
             * 1: L     //magic
             * 2: Z     //magic
             * 3: A/P   //arithm / ppm
             * 4: C/F/A //model: custom/fixed/adaptive
             * [ASHigh] //adaptive only: high byte of adaptive block size
             * [ASLow]  //adaptive only: low byte of adaptive block size
             * [CTable ...] //Custom Freq Table (32 bit big-endian * 256)
             * S3       //total inflated size big-endian
             * S2
             * S1
             * S0
             * [Data ...]   //Compressed code
             */

            std::vector<char> output;
            output.push_back('R'); //magic
            output.push_back('L');
            output.push_back('Z');
            output.push_back('A'); //arithm
            if(compressor.compMode == RLZCompressor::COMP_MODEL_CUSTOM) {
                output.push_back('C'); //custom
            } else if(compressor.compMode == RLZCompressor::COMP_MODEL_FIXED) {
                output.push_back('F'); //fixed
            } else if(compressor.compMode == RLZCompressor::COMP_MODEL_ADAPTIVE) {
                output.push_back('A'); //adaptive
                output.push_back((compressor.adaptiveBlockSize >> 8) & 0xff ); //block size High byte
                output.push_back((compressor.adaptiveBlockSize) & 0xff ); //block size Low byte
            }

            if(compressor.compMode == RLZCompressor::COMP_MODEL_CUSTOM) {
                std::vector<char> table = compressor.packTable();
                output.insert(output.end(), table.begin(), table.end());
            }

            //length
            for(int k = 3; k >=0; k--) {
                int val = (sz >> (k*8)) & 0xff;
                output.push_back(val);
            }


            output.insert(output.end(), compressor.result.begin(), compressor.result.end());

            writeToFile(argv[3], output);


        }
    } else if(mode == MODE_GENERATE_MODEL) {
        RLZCompressor compressor;

        for(int i = 2; i<argc; i++) {
            std::vector<char> tData;
            size_t sz = loadFileToMem(argv[i], tData);
            printf("File %s loaded, size %lu\n", argv[i], sz);
            compressor.data.insert(compressor.data.end(), tData.begin(), tData.end());
        }
        compressor.buildModel();

        printf("Model generated\n ===== Cut Here ===== \n");
        printf("double staticFreqs[] = { \n");
        for(int i = 0; i<compressor.freqs.size(); i++) {
            printf(" /* %d */ %f, ", i, compressor.freqs[i]);
        }
        printf(" 0 }; \n");
    } else if(mode == MODE_DECOMPRESS) {
        RLZCompressor compressor;

        std::vector<char> data;
        size_t sz = loadFileToMem(argv[2], data);
        printf("File %s loaded, size %lu\n", argv[2], sz);

        if(sz) {

            //parse header. I know, sequental std::vector.erase(begin) is really bad, no time to optimize right now

            if(memcmp(data.data(), "RLZ", 3)) {
                printf("Bad file - magic not found!\n");
                exit(1);
            }
            if(data[3] != 'A') {
                printf("Bad file - unsupported compression method!\n");
                exit(1);
            }
            if(data[4] == 'C') {
                compressor.compMode = RLZCompressor::COMP_MODEL_CUSTOM;
                std::vector<char> tPacked(data.begin() + 5, data.begin() + 5 + 256 * 4);
                compressor.unpackTable(tPacked);
                data.erase(data.begin(), data.begin() + 5 + 256 * 4);
            } else if(data[4] == 'F') {
                compressor.compMode = RLZCompressor::COMP_MODEL_FIXED;
                data.erase(data.begin(), data.begin() + 4);
            } else if(data[4] == 'A') {
                compressor.compMode = RLZCompressor::COMP_MODEL_ADAPTIVE;
                compressor.adaptiveBlockSize = (data[5] << 8) + data[6];
                data.erase(data.begin(), data.begin() + 6);
            } else {
                printf("Bad file - unsupported freq table method!\n");
                exit(1);
            }

            size_t length = 0;
            int lengthIdx = 0;
            //length
            for(int k = 3; k >=0; k--) {
                length |= (data[lengthIdx] << (k*8));
                lengthIdx++;
            }
            data.erase(data.begin(), data.begin()+3);
            compressor.normModel();
            compressor.data = data;

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

            compressor.decompress(length);
            printf("\n");
        }


    }
    return 0;
}

