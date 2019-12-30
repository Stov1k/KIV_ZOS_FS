//
// Created by pavel on 29.12.19.
//

#include <cstdint>
#include <fstream>
#include <bitset>
#include <iostream>
#include "datablock.h"
#include "zosfsstruct.h"

/**
 * Vrati volnou pozici databloku
 * @param buf buffer - pozice v bloku, pozice v byte, bitmap byte, poradi bytu
 * @return vrati, zdali existuje volna pozice
 */
bool getFreeDatablock(filesystem& filesystem_data, int32_t buf[]) {
    std::fstream input_file;
    input_file.open(filesystem_data.fs_file, std::ios::in);

    input_file.seekp(filesystem_data.super_block.bitmap_start_address);
    bool found = false;
    uint8_t bitmap_byte;
    int bitmap_size_bytes = (filesystem_data.super_block.inode_start_address - filesystem_data.super_block.bitmap_start_address);
    for(int i = 0; i < bitmap_size_bytes; i++) {
        char b;
        input_file.read(&b, 1);

        // bitova mnozina reprezentujici 8 databloku
        std::bitset<8> x(b);

        // vyhledani nuloveho bitu
        for(int j = 0; j < 8; j++) {
            bool used = x.test(j);
            if(!found && !used) {
                found = true;
                buf[1] = j;
            }
            std::cout << j << " " << used << std::endl;
        }
        std::cout << "Pozice " << buf[1] << std::endl;

        // zmena bitu
        if(found) {
            x.set(buf[1], true);   // zmeni cislo na pozici na 1
        }
        unsigned long ul = x.to_ulong();
        unsigned int c = static_cast<unsigned int>(ul);
        bitmap_byte = c;
        std::cout << x << " pos:" << input_file.tellg() << " Long:" << ul << " Int:" << c << std::endl;

        // konec cyklu
        if(found) {
            buf[0] = buf[1] + (8*i);
            buf[2] = bitmap_byte;
            buf[3] = i;
            break;
        }
    }

    input_file.close();
    if(found) {
        return true;
    }
    return false;
}