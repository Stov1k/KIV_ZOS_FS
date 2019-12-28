//
// Created by pavel on 28.12.19.
//

#include <fstream>
#include "inode.h"
#include "zosfsstruct.h"

/**
 * Vrati volny inode
 * @return inode
 */
pseudo_inode * getFreeINode(filesystem &filesystem_data) {
    std::fstream input_file;
    input_file.open(filesystem_data.fs_file, std::ios::in);

    input_file.seekp(filesystem_data.super_block.inode_start_address);

    // Pocet inodu jako misto mezi zacatkem datove casti a zacatkem prvniho inodu
    int32_t inodes_count = (filesystem_data.super_block.data_start_address - filesystem_data.super_block.inode_start_address) / sizeof(pseudo_inode);

    // nactu pole inodu
    pseudo_inode inodes[inodes_count];
    input_file.read(reinterpret_cast<char *>(&inodes), sizeof(inodes));

    // Hledam inode
    pseudo_inode inode;
    pseudo_inode * inode_ptr = nullptr;
    for(int i = 0; i < inodes_count; i++) {
        if(inodes[i].nodeid == 0) {     // volny inode ma ID == 0
            inode = inodes[i];
            inode.nodeid = (i+1);       // pridam inodu poradove cislo jako ID
            inode_ptr = &inode;
            break;
        }
    }

    input_file.close();
    return inode_ptr;
}
