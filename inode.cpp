//
// Created by pavel on 28.12.19.
//

#include <fstream>
#include <vector>
#include <iostream>
#include "inode.h"
#include "zosfsstruct.h"
#include "directory.h"

/**
 * Vrati pozici inodu v souboru podle poradi inodu
 * @param filesystem_data filesystem
 * @param inode_no poradi inodu
 * @return pozice v souboru fs
 */
int32_t getINodePosition(filesystem &filesystem_data, int32_t inode_no) {
    return filesystem_data.super_block.inode_start_address +
           (inode_no - 1) * sizeof(pseudo_inode);
}

/**
 * Vrati volny inode
 * @return inode
 */
pseudo_inode *getFreeINode(filesystem &filesystem_data) {
    std::fstream input_file;
    input_file.open(filesystem_data.fs_file, std::ios::in);

    input_file.seekp(filesystem_data.super_block.inode_start_address);

    // Pocet inodu jako misto mezi zacatkem datove casti a zacatkem prvniho inodu
    int32_t inodes_count =
            (filesystem_data.super_block.data_start_address - filesystem_data.super_block.inode_start_address) /
            sizeof(pseudo_inode);

    // nactu pole inodu
    pseudo_inode inodes[inodes_count];
    input_file.read(reinterpret_cast<char *>(&inodes), sizeof(inodes));

    // Hledam inode
    pseudo_inode inode;
    pseudo_inode *inode_ptr = nullptr;
    for (int i = 0; i < inodes_count; i++) {
        if (inodes[i].nodeid == 0) {     // volny inode ma ID == 0
            inode = inodes[i];
            inode.nodeid = (i + 1);       // pridam inodu poradove cislo jako ID
            inode_ptr = &inode;
            break;
        }
    }

    input_file.close();
    return inode_ptr;
}

/**
 * Vrati referenci na inode souboru
 * @param s1 nazev souboru
 */
pseudo_inode *getFileINode(filesystem &filesystem_data, std::string &s1) {
    std::fstream input_file;
    input_file.open(filesystem_data.fs_file, std::ios::in | std::ios::out | std::ios::binary);

    pseudo_inode inode;
    pseudo_inode *inode_ptr = nullptr;

    // zjistim, zdali existuje adresar stejneho nazvu
    std::vector<directory_item> directories = getDirectories(filesystem_data, filesystem_data.current_dir);
    for (auto &directory : directories) {
        directory.item_name;
        if (strcmp(s1.c_str(), directory.item_name) == 0) {
            input_file.seekp(getINodePosition(filesystem_data, directory.inode));
            input_file.read(reinterpret_cast<char *>(&inode), sizeof(pseudo_inode));
            if (inode.isDirectory) {
                std::cout << "FILE IS DIRECTORY" << std::endl;
            } else {
                std::cout << "OBSAH" << std::endl;
                inode_ptr = &inode;
            }
            break;
        }
    }

    input_file.close();
    return inode_ptr;
}