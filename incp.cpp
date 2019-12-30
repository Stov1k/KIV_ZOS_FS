//
// Created by pavel on 28.12.19.
//

#include <string>
#include <sys/stat.h>
#include <fstream>
#include <bitset>
#include <iostream>
#include <cstring>
#include "incp.h"
#include "zosfsstruct.h"
#include "inode.h"
#include "directory.h"
#include "datablock.h"

/**
 * Spocte pocet volnych databloku z bitmapy
 * @param filesystem_data filesystem
 * @return pocet volnych databloku
 */
long countFreeDatablock(filesystem &filesystem_data) {
    std::fstream input_file;
    input_file.open(filesystem_data.fs_file, std::ios::in);

    input_file.seekp(filesystem_data.super_block.bitmap_start_address);

    int bitmap_size_bytes = (filesystem_data.super_block.inode_start_address - filesystem_data.super_block.bitmap_start_address);
    long found = 0;

    for(int i = 0; i < bitmap_size_bytes; i++) {
        char b;
        input_file.read(&b, 1);

        // bitova mnozina reprezentujici 8 databloku
        std::bitset<8> x(b);

        // vyhledani nuloveho bitu
        for (int i = 0; i < 8; i++) {
            bool used = x.test(i);
            if (!used) {
                found = found + 1;
            }
            std::cout << i << " " << found << std::endl;
        }
    }

    input_file.close();

    return found;
}

/**
 * Vrati velikost souboru mimo FS
 * @param filename umisteni
 * @return velikost souboru
 */
long getFilesize(std::string &filename) {
    struct stat stat_buf;
    int rc = stat(filename.c_str(), &stat_buf);
    return rc == 0 ? stat_buf.st_size : -1;
}

void writeDatablock(filesystem &filesystem_data, int32_t * datablock, std::ifstream &file_input, std::fstream &input_file, char * buffer) {
    // ziskani datablocku
    getFreeDatablock(filesystem_data, datablock);
    int32_t position_absolute = datablock[0];
    int32_t position_byte = datablock[3];           // poradi bytu v bitmape
    uint8_t bitmap_byte = (uint8_t) datablock[2];
    // nacteni dat do bufferu k zapisu
    file_input.read(buffer, filesystem_data.super_block.cluster_size);
    // aktualizace bitmapy
    input_file.seekp(filesystem_data.super_block.bitmap_start_address + position_byte); // skoci na bitmapu
    input_file.write(reinterpret_cast<const char *>(&bitmap_byte), sizeof(bitmap_byte));    // zapise upravenou bitmapu
    // aktualizace dat
    input_file.seekp(filesystem_data.super_block.data_start_address + (position_absolute * filesystem_data.super_block.cluster_size));
    input_file.write(reinterpret_cast<const char *>(&buffer), filesystem_data.super_block.cluster_size);
    memset(buffer, 0, filesystem_data.super_block.cluster_size);        // vymaze obsah bufferu
}

/**
 * Zapis na FS
 * @param filesystem_data filesystem
 * @param input vstupni soubor
 * @param location umisteni na FS
 */
void inputCopy(filesystem &filesystem_data, std::string &input, std::string &location) {

    // zjisti, zdali jiz soubor stejneho nazvu existuje
    directory_item dir = getDirectory(0, location); // TODO: location pak predelat (bude moc byt uvadena cesta)
    if(isDirectoryExists(filesystem_data, dir)) {
        std::cout << "EXIST" << std::endl;
        return;
    }

    // spocte velikost input
    long filesize = getFilesize(input);

    // zjisti dostupnou velikost
    long free_space = countFreeDatablock(filesystem_data);
    free_space = free_space * filesystem_data.super_block.cluster_size;
    if(free_space < filesize) {
        std::cout << "NOT ENOUGH SPACE (FREE: " << free_space << " REQUIRED: " << filesize << ")" << std::endl;
        return;
    }

    // zajisti volny inode
    pseudo_inode * inode_ptr = getFreeINode(filesystem_data);
    pseudo_inode inode;
    if(inode_ptr != nullptr) {
        inode = *inode_ptr;
    } else {
        std::cout << "FREE INODE NOT FOUND" << std::endl;
        return;
    }

    // vytvor adresar
    std::fstream input_file;
    input_file.open(filesystem_data.fs_file, std::ios::in | std::ios::out | std::ios::binary);
    input_file.seekp(filesystem_data.current_dir.direct1);
    uint32_t dirs_per_cluster = filesystem_data.super_block.cluster_size / sizeof(directory_item);
    directory_item dirs[dirs_per_cluster];
    input_file.read(reinterpret_cast<char *>(&dirs), sizeof(dirs));
    for(int i = 0; i < dirs_per_cluster; i++) {
        if(!dirs[i].inode) {
            std::strncpy(dirs[i].item_name, dir.item_name, sizeof(dir.item_name));
            dirs[i].inode = inode.nodeid;
            break;
        }
    }

    std::ifstream file_input(input, std::ios::in | std::ios::binary );
    // provest zapis
    int writed = 0;
    char buffer[filesystem_data.super_block.cluster_size];
    int32_t datablock[4];

    // 1P - prvni datablock
    writeDatablock(filesystem_data,datablock,file_input,input_file,buffer);
    inode.direct1 = filesystem_data.super_block.data_start_address + (datablock[0] * filesystem_data.super_block.cluster_size);;
    writed += sizeof(buffer);

    // 2P - druhy datablock
    if(writed < filesize) {
        writeDatablock(filesystem_data, datablock, file_input, input_file, buffer);
        inode.direct2 = filesystem_data.super_block.data_start_address +
                        (datablock[0] * filesystem_data.super_block.cluster_size);;
        writed += sizeof(buffer);
    }

    // 3P - treti datablock
    if(writed < filesize) {
        writeDatablock(filesystem_data, datablock, file_input, input_file, buffer);
        inode.direct3 = filesystem_data.super_block.data_start_address +
                        (datablock[0] * filesystem_data.super_block.cluster_size);;
        writed += sizeof(buffer);
    }

    // 4P - ctvrty datablock
    if(writed < filesize) {
        writeDatablock(filesystem_data, datablock, file_input, input_file, buffer);
        inode.direct4 = filesystem_data.super_block.data_start_address +
                        (datablock[0] * filesystem_data.super_block.cluster_size);;
        writed += sizeof(buffer);
    }

    // 5P - paty datablock
    if(writed < filesize) {
        writeDatablock(filesystem_data, datablock, file_input, input_file, buffer);
        inode.direct5 = filesystem_data.super_block.data_start_address +
                        (datablock[0] * filesystem_data.super_block.cluster_size);;
        writed += sizeof(buffer);
    }

    // 1N - prvni neprimy
    // 2N - druhy neprimy




    //while(filesize > writed) {

    //}

    // aktualizace bitmapy
    //input_file.seekp(filesystem_data.super_block.bitmap_start_address+position_byte); // skoci na bitmapu
    //input_file.write(reinterpret_cast<const char *>(&bitmap_byte), sizeof(bitmap_byte));    // zapise upravenou bitmapu

    // aktualizace podslozek v nadrazenem adresari
    input_file.seekp(filesystem_data.current_dir.direct1); // skoci na data
    input_file.write(reinterpret_cast<const char *>(&dirs), sizeof(dirs));

    // zapis inode
    input_file.seekp(filesystem_data.super_block.inode_start_address + (inode.nodeid-1) * sizeof(pseudo_inode));
    inode.isDirectory = false;
    inode.references++;
    inode.indirect1 = 0;
    inode.indirect2 = 0;
    input_file.write(reinterpret_cast<const char *>(&inode), sizeof(pseudo_inode));

    input_file.close();
}


/**
 * Zapis na FS
 * @param input vstupni soubor
 * @param location umisteni na FS
 *
void incp(std::string &input, std::string &location) {
    std::fstream input_file;

    // novy adresar
    directory_item dir = getDirectory(0, location);

    // nalezeny volny inode
    pseudo_inode inode;

    // zjistim, zdali jiz neexistuje adresar stejneho nazvu
    if(isDirectoryExists(dir)) {
        std::cout << "EXIST" << std::endl;
        return;
    }

    long size = filesize(input);
    std::cout << size << std::endl;

    // Otevreni pro cteni i zapis
    input_file.open(input_file_name, std::ios::in | std::ios::out | std::ios::binary);

    int free = countFreePositions() * super_block.cluster_size;
    std::cout << free << std::endl;

    if(size < free) {

        // najdu volny inode
        inode = getFreeNode();

        int writed = 0;
        char buffer[super_block.cluster_size];
        std::ifstream file_input(input, std::ios::in | std::ios::binary );
        int32_t datablock[3];
        while(size > writed) {
            getFreePosition(datablock);
            int32_t position = datablock[0];
            uint8_t bitmap_byte = (uint8_t) datablock[2];

            file_input.read(buffer, super_block.cluster_size);

            // aktualizace bitmapy
            input_file.seekp(super_block.bitmap_start_address); // skoci na bitmapu
            input_file.write(reinterpret_cast<const char *>(&bitmap_byte), sizeof(bitmap_byte));    // zapise upravenou bitmapu

            writed += super_block.cluster_size;
        }
        file_input.close();
    }

    input_file.close();
}
 */