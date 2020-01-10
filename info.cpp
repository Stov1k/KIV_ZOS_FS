//
// Created by pavel on 06.01.20.
//

#include <fstream>
#include <vector>
#include <iostream>
#include "info.h"
#include "zosfsstruct.h"
#include "directory.h"
#include "inode.h"
#include "datablock.h"

/**
 * Vypise adresy databloku
 * @param filesystem_data filesystem
 * @param fs_file otevreny soubor filesystemu
 * @param inode soubor
 */
void printDatablocksAddresses(filesystem &filesystem_data, std::fstream &fs_file, pseudo_inode &inode) {
    // platne adresy na databloky
    std::vector<int32_t> addresses = usedDatablockByINode(filesystem_data, fs_file, inode, true);
    std::cout << addresses.size() << " BLOCKS | ";
    // prochazeni adres databloku
    for (auto &address : addresses) {
        std::cout << address << " ";
    }
}

/**
 * Vypise informace o souboru/adresari s1/a1
 * @param filesystem_data filesystem
 * @param name nazev
 */
void info(filesystem &filesystem_data, std::string &name) {
    std::fstream fs_file;
    fs_file.open(filesystem_data.fs_file, std::ios::in | std::ios::out | std::ios::binary);

    pseudo_inode *inode_ptr = nullptr;
    pseudo_inode inode;
    std::string dir_name;

    // zjistim, zdali existuje adresar stejneho nazvu
    std::vector<directory_item> directories = getDirectories(filesystem_data, filesystem_data.current_dir);
    for (auto &directory : directories) {
        if (strcmp(name.c_str(), directory.item_name) == 0) {
            dir_name = directory.item_name;
            fs_file.seekp(getINodePosition(filesystem_data, directory.inode));
            fs_file.read(reinterpret_cast<char *>(&inode), sizeof(pseudo_inode));
            inode_ptr = &inode;
            break;
        }
    }

    if (inode_ptr != nullptr) {
        std::cout << "NAME\t – SIZE\t – INODE\t – LINKS" << std::endl;
        std::cout << dir_name << "\t – " << inode.file_size << "\t – " << inode.nodeid << "\t – ";
        printDatablocksAddresses(filesystem_data, fs_file, inode);
        std::cout << std::endl;
    } else {
        std::cout << "FILE NOT FOUND" << std::endl;
    }
    fs_file.close();
}