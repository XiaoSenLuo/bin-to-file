#include <iostream>
#include <cstdlib>
#include <stdio.h>
#include <stdint.h>
#include <string>
#include <string.h>
#include <stdlib.h>
#include <fstream>

#include <vector>
#include "CRCpp-release-1.1.0.0/inc/CRC.h"
#include "argparse-2.2/include/argparse/argparse.hpp"

using  namespace std;

static __inline int32_t complement_to_decimal(uint32_t com, uint8_t bits){
    return ((com & (0x00000001 << (bits - 1))) ? (int32_t)(com | (~((1 << bits) - 1))) : (int32_t)(com));
}

static __inline uint32_t decimal_to_complement(int32_t dec, uint8_t bits){
    return (((dec) & (0x00000001 << (bits - 1))) ? (uint32_t)((((0x01 << bits) - 1) & (dec)) | (0x01 << (bits - 1))) : (uint32_t)(dec));
}

static __inline time_t parse_time(uint32_t data){
    struct tm atime = {0};

    atime.tm_year = (int)((data >> 26) & 63) + 2020 - 1900;
    atime.tm_mon = (int)((data >> 22) & 15) - 1;
    atime.tm_mday = (int)((data >> 17) & 31);
    atime.tm_hour = (int)((data >> 12) & 31);
    atime.tm_min = (int)((data >> 6) & 63);
    atime.tm_sec = (int)(data & 63);

    return mktime(&atime);
}

int main(int argc, char* *argv) {
    argparse::ArgumentParser argumentParser("bin to csv");
    argumentParser.add_argument("-v", "--version").help("binary file to csv file").action([=](const string& s){
        std::cout << "Version 0.0.1 bate";
        std::exit(0);
    })
    .default_value(false)
    .help("Show Version Message")
    .implicit_value(true).nargs(0);

    argumentParser.add_argument("-f", "--file").help("Input Binary File Path");

    argumentParser.add_argument("-c", "--channel").help("Set Channel Number").default_value(2).scan<'i', int>();

    argumentParser.add_argument("-h", "--help").action([=](const string& s){
        std::cout << argumentParser.help().str();
        std::exit(0);
    })
    .default_value(false)
    .help("Show Help Message")
    .implicit_value(true)
    .nargs(0);

    string file_path;
    int channel = 2;

    try{
        argumentParser.parse_args(argc, argv);
    } catch (std::logic_error& e) {
        std::cerr << e.what() << std::endl;
        std::cerr << argumentParser.help().str();
        std::exit(-1);
    }

    try{
        file_path = argumentParser.get<std::string>("-f");
    } catch (std::logic_error& e) {
        std::cerr << "No file provided" << std::endl;
        std::exit(-1);
    }

    try{
        channel = argumentParser.get<int>("-c");
    } catch (std::logic_error& e) {
//        std::cerr << "" << std::endl;
        std::exit(-1);
    }

    ifstream bin_file;
    ofstream csv_file[2];
    streampos ifile_start = 0, ifile_end = 0, ifile_size = 0;

    bin_file = ifstream(file_path, ios::in|ios::binary);

    if(bin_file.good()){
        string csv_path;
        for(int c = 0; c < channel; c++){
            csv_path = file_path + string("[") + to_string(c) + string("]") + string(".csv");
            csv_file[c] = ofstream(csv_path.data());
        }
    }else{
        return -1;
    }

    ifile_start = bin_file.tellg();
    bin_file.seekg(0, ios::end);
    ifile_end = bin_file.tellg();
    ifile_size = ifile_end - ifile_start;

    bin_file.seekg (0, ios::beg);

    for(int c = 0 ; c < channel; c++){
        if(!csv_file[c].is_open()){
            return -1;
        }
    }
    char *read_data_buffer = NULL;
    int read_size = int(ifile_size);
    read_data_buffer = new char [read_size];

    bin_file.read(read_data_buffer, read_size);
//    std::vector<unsigned char> read_data_buffer(std::istreambuf_iterator<char>(bin_file), {});
    bin_file.close();

    long long int i = 0;
    long int offset = 32008;
    uint32_t tmp[2] = {0};
    int32_t data[2] = {0};
    time_t time = 0;
    uint32_t index = 0;
    char cbuf[128] = {'\0'};
    string sstr[2];

    for(i = 0; i < ifile_size / (offset - 8); i++){  // i * 0ffset
        tmp[0] = *(uint32_t*)(&read_data_buffer[i * offset]); // time
        time = parse_time(tmp[0]);
        tmp[1] = *(uint32_t*)(&read_data_buffer[i * offset + 4]); // index
        index = tmp[1];
        snprintf(cbuf, sizeof(cbuf), "%ld,%ld", time, index);
        sstr[0].append(cbuf);
        sstr[1].append(cbuf);
        for(int j = 8; j < offset; j += (channel * 4)){
            for(int c = 0; c < channel; c++){
                tmp[c] = (((uint8_t)read_data_buffer[i * offset + j + c * 4]) << 16)
                         | (((uint8_t)read_data_buffer[i * offset + j + c * 4 + 1]) << 8)
                         | ((uint8_t)read_data_buffer[i * offset + j + c * 4  + 2]);
                data[c] = complement_to_decimal(tmp[c], 24);
                snprintf(cbuf, sizeof(cbuf), ",%d", data[c]);
                sstr[c].append(cbuf);
            }
        }
        for(int c  = 0; c < channel; c++){
            sstr[c].append("\n");
            csv_file[c].write(sstr[c].data(), sstr[c].length());
            sstr[c].clear();
        }
    }

    delete[] read_data_buffer;

    for(int c = 0; c < channel; c++){
        csv_file[c].close();
    }

    return 0;
}
