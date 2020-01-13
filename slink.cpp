//
// Created by pavel on 13.01.20.
//

#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include "slink.h"
#include "zosfsstruct.h"
#include "inode.h"
#include "cd.h"
#include "directory.h"
#include "datablock.h"
#include "pwd.h"

/**
 * Vytvori symbolicky link na soubor s1 s nazvem s2
 * @param filesystem_data filesystem
 * @param s1 nazev souboru
 * @param s2 nazev souboru
 */
void slink(filesystem &filesystem_data, std::string &s1, std::string &s2) {

    // zdrojovy soubor
    pseudo_inode s1_inode;
    pseudo_inode *s1_inode_ptr = iNodeByLocation(filesystem_data, s1, false);
    if (nullptr != s1_inode_ptr) {
        s1_inode = *s1_inode_ptr;
    } else {
        std::cout << "FILE NOT FOUND" << std::endl;     // SOURCE FILE DOES NOT EXISTS
        return;
    }

    // nadrazeny adresar
    pseudo_inode a2_parrent_inode;
    pseudo_inode *a2_parrent_inode_ptr = cd(filesystem_data, s2, false, false);
    if (a2_parrent_inode_ptr != nullptr) {
        a2_parrent_inode = *a2_parrent_inode_ptr;
        if (a2_parrent_inode.type != 1) {
            std::cout << "FILE IS NOT DIRECTORY" << std::endl;
            return;
        }
    } else {
        std::cout << "PATH NOT FOUND" << std::endl;  // PARRENT DIRECTORY DOES NOT EXISTS
        return;
    }

    // soubor jiz existuje
    if (nullptr != iNodeByLocation(filesystem_data, s2, false)) {
        std::cout << "EXIST" << std::endl;
        return;
    }

    // nalezeny volny inode
    pseudo_inode s2_inode;
    pseudo_inode *s2_inode_ptr = getFreeINode(filesystem_data);
    if (s2_inode_ptr != nullptr) {
        s2_inode = *s2_inode_ptr;
        s2_inode.type = 2;
        s2_inode.file_size = filesystem_data.super_block.cluster_size;
    } else {
        std::cout << "NO FREE INODE LEFT" << std::endl;
        return;
    }

    std::fstream fs_file;
    fs_file.open(filesystem_data.fs_file, std::ios::in | std::ios::out | std::ios::binary);

    // zapis inode
    fs_file.seekp(getINodePosition(filesystem_data, s2_inode.nodeid));
    fs_file.write(reinterpret_cast<const char *>(&s2_inode), sizeof(pseudo_inode));

    // zapis zaznamu adresare v nadrazenem adresari
    std::vector<std::string> s2_segments = splitPath(s2);
    directory_item target_dir = createDirectoryItem(s2_inode.nodeid, s2_segments.back());
    int32_t address = addDirectoryItemEntry(filesystem_data, a2_parrent_inode, target_dir);
    if (!address) {
        std::cout << "MAXIMUM NUMBER OF FILES REACHED" << std::endl;
        removeINode(filesystem_data, fs_file, s2_inode);
        fs_file.close();
        return;
    }

    // pridam datablok s odkazem
    int32_t obtained_address = addDatablockToINode(filesystem_data, fs_file, s2_inode);
    int32_t cluster_size = filesystem_data.super_block.cluster_size;
    char buffer[cluster_size];
    memset(buffer, EOF, cluster_size);
    std::string path = s1;
    path.copy(buffer, cluster_size);

    // aktualizace dat
    fs_file.seekp(s2_inode.direct1);
    fs_file.write(reinterpret_cast<const char *>(&buffer), cluster_size);

    fs_file.close();

    std::cout << "OK" << std::endl;
}