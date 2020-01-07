//
// Created by pavel on 08.01.20.
//

#include <fstream>
#include <iostream>
#include "mkdir.h"
#include "zosfsstruct.h"
#include "directory.h"
#include "datablock.h"
#include "inode.h"
#include "cd.h"

/**
 * Vytvori adresar a1
 * @param a1 nazev adresare
 */
void mkdir(filesystem &filesystem_data, std::string &a1) {

    std::fstream fs_file;
    fs_file.open(filesystem_data.fs_file, std::ios::in | std::ios::out | std::ios::binary);

    // novy adresar
    directory_item dir = getDirectory(0, a1);

    // nalezeny volny inode
    pseudo_inode inode;
    pseudo_inode *inode_ptr;

    // zjistim, zdali jiz neexistuje adresar stejneho nazvu
    if (isDirectoryExists(filesystem_data, dir)) {
        fs_file.close();
        std::cout << "EXIST" << std::endl;
        return;
    }

    // najdu volny inode
    inode_ptr = getFreeINode(filesystem_data);
    if (inode_ptr != nullptr) {
        inode = *inode_ptr;
    } else {
        fs_file.close();
        std::cout << "FREE INODE NOT FOUND" << std::endl;
        return;
    }

    // najdu v bitmape volny datablok
    int32_t datablock_buf[4];
    bool found = getFreeDatablock(filesystem_data, datablock_buf);
    if (!found) {
        std::cout << "DISK IS FULL" << std::endl;
        return;
    }
    int32_t position_absolute = datablock_buf[0];       // poradi bitu od zacatku bitmapy
    int32_t position_byte = datablock_buf[3];           // poradi bytu v bitmape
    uint8_t bitmap_byte = (uint8_t) datablock_buf[2];

    //v korenovem adresari vytvorim polozku dir(nazev, inode)
    fs_file.seekp(filesystem_data.current_dir.direct1);
    uint32_t dirs_per_cluster = filesystem_data.super_block.cluster_size / sizeof(directory_item);
    directory_item dirs[dirs_per_cluster];
    fs_file.read(reinterpret_cast<char *>(&dirs), sizeof(dirs));
    for (int i = 0; i < dirs_per_cluster; i++) {
        if (!dirs[i].inode) {
            std::strncpy(dirs[i].item_name, dir.item_name, sizeof(dir.item_name));
            dirs[i].inode = inode.nodeid;
            break;
        }
    }

    // aktualizace bitmapy
    fs_file.seekp(filesystem_data.super_block.bitmap_start_address + position_byte); // skoci na bitmapu
    fs_file.write(reinterpret_cast<const char *>(&bitmap_byte), sizeof(bitmap_byte));    // zapise upravenou bitmapu

    // aktualizace podslozek v nadrazenem adresari
    fs_file.seekp(filesystem_data.current_dir.direct1); // skoci na data
    fs_file.write(reinterpret_cast<const char *>(&dirs), sizeof(dirs));

    // zapis inode
    fs_file.seekp(filesystem_data.super_block.inode_start_address + (inode.nodeid - 1) * sizeof(pseudo_inode));
    inode.isDirectory = true;
    inode.references++;
    inode.direct1 = filesystem_data.super_block.data_start_address +
                    (position_absolute * filesystem_data.super_block.cluster_size);
    inode.direct2 = 0;
    inode.direct3 = 0;
    inode.direct4 = 0;
    inode.direct5 = 0;
    inode.indirect1 = 0;
    inode.indirect2 = 0;
    inode.file_size = filesystem_data.super_block.cluster_size;
    fs_file.write(reinterpret_cast<const char *>(&inode), sizeof(pseudo_inode));

    // aktualni adresar
    directory_item dir_dot;
    dir_dot = getDirectory(inode.nodeid, ".");

    // predchozi adresar
    directory_item dir_parrent;
    dir_parrent = getDirectory(filesystem_data.current_dir.nodeid, "..");

    directory_item subdirectories[dirs_per_cluster];
    subdirectories[0] = dir_dot;                       // .
    subdirectories[1] = dir_parrent;                   // ..
    for (int i = 2; i < dirs_per_cluster; i++) {        // dalsi neobsazene
        subdirectories[i] = directory_item{};
        subdirectories[i].inode = 0;   // tj. nic
    }

    // aktualizace podslozek v novem adresari
    fs_file.seekp(filesystem_data.super_block.data_start_address +
                     (position_absolute * filesystem_data.super_block.cluster_size)); // skoci na data
    fs_file.write(reinterpret_cast<const char *>(&subdirectories), sizeof(subdirectories));

    fs_file.close();
    std::cout << "OK" << std::endl;
}