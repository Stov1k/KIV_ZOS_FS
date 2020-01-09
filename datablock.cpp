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
 * Pocet odkazu, ktere lze ulozit do jednoho databloku
 * @param filesystem_data filesystem
 * @return pocet odkazu
 */
int32_t linksPerCluster(filesystem &filesystem_data) {
    return filesystem_data.super_block.cluster_size / sizeof(int32_t);
}

/**
 * Vrati volnou pozici databloku
 * @param buf buffer - pozice v bloku, pozice v byte, bitmap byte, poradi bytu
 * @return vrati, zdali existuje volna pozice
 */
bool getFreeDatablock(filesystem &filesystem_data, int32_t buf[]) {
    std::fstream input_file;
    input_file.open(filesystem_data.fs_file, std::ios::in);

    input_file.seekp(filesystem_data.super_block.bitmap_start_address);
    bool found = false;
    uint8_t bitmap_byte;
    int bitmap_size_bytes = (filesystem_data.super_block.inode_start_address -
                             filesystem_data.super_block.bitmap_start_address);
    for (int i = 0; i < bitmap_size_bytes; i++) {
        char b;
        input_file.read(&b, 1);

        // bitova mnozina reprezentujici 8 databloku
        std::bitset<8> x(b);

        // vyhledani nuloveho bitu
        for (int j = 0; j < 8; j++) {
            bool used = x.test(j);
            if (!found && !used) {
                found = true;
                buf[1] = j;
            }
        }

        // zmena bitu
        if (found) {
            x.set(buf[1], true);   // zmeni cislo na pozici na 1
        }
        unsigned long ul = x.to_ulong();
        unsigned int c = static_cast<unsigned int>(ul);
        bitmap_byte = c;

        // konec cyklu
        if (found) {
            buf[0] = buf[1] + (8 * i);
            buf[2] = bitmap_byte;
            buf[3] = i;
            break;
        }
    }

    input_file.close();
    if (found) {
        return true;
    }
    return false;
}

/**
 * Vytvoreni datablocku neprimych odkazu
 * @param filesystem_data filesystem
 * @param links_per_cluster pocet odkazu na datablock
 * @param cpin_file otevreny soubor na pevnem disku
 * @param fs_file otevreny soubor filesystemu
 * @return pozice databloku
 */
int32_t createIndirectDatablock(filesystem &filesystem_data, std::fstream &fs_file) {
    int32_t links_per_cluster = linksPerCluster(filesystem_data);
    int32_t links[links_per_cluster];
    for (int i = 0; i < links_per_cluster; i++) {
        links[i] = 0;
    }

    // ziskani datablocku (odkazy)
    int32_t datablock[4];
    getFreeDatablock(filesystem_data, datablock);
    int32_t position_byte = datablock[3];           // poradi bytu v bitmape
    uint8_t bitmap_byte = (uint8_t) datablock[2];

    // aktualizace bitmapy (odkazy)
    fs_file.seekp(filesystem_data.super_block.bitmap_start_address + position_byte); // skoci na bitmapu
    fs_file.write(reinterpret_cast<const char *>(&bitmap_byte), sizeof(bitmap_byte));    // zapise upravenou bitmapu

    // aktualizace dat (odkazy)
    int32_t datablock_position =
            filesystem_data.super_block.data_start_address + (datablock[0] * filesystem_data.super_block.cluster_size);
    fs_file.seekp(datablock_position);
    fs_file.write(reinterpret_cast<const char *>(&links), filesystem_data.super_block.cluster_size);

    return datablock_position;
}

