//
// Created by pavel on 05.01.20.
//

#include <string>
#include <fstream>
#include <vector>
#include <iostream>
#include "rm.h"
#include "zosfsstruct.h"
#include "directory.h"
#include "inode.h"
#include "datablock.h"
#include "cd.h"

/**
 * Vrati, zdali adresar je prazdny
 * @param filesystem_data filesystem
 * @param fs_file otevreny soubor filesystemu
 * @param inode adresar
 * @return je prazdny?
 */
bool isDirectoryEmpty(filesystem &filesystem_data, std::fstream &fs_file, pseudo_inode &inode) {

    // adresare v datablocku
    uint32_t dirs_per_cluster = filesystem_data.super_block.cluster_size / sizeof(directory_item);
    directory_item directories[dirs_per_cluster];

    // platne adresy na databloky
    std::vector<int32_t> addresses = usedDatablockByINode(filesystem_data, fs_file, inode, false);

    // prochazeni adres databloku
    for (auto &address : addresses) {
        // pruchod adresari
        fs_file.seekp(address);
        fs_file.read(reinterpret_cast<char *>(&directories), sizeof(directories));

        int i = 0;
        if (address == inode.direct1) i = 2;     // vyjimka pro prvni odkaz (reference na . a ..)
        for (i; i < dirs_per_cluster; i++) {
            if (directories[i].inode) return false;
        }
    }
    return true;
}

/**
 * Odstraneni adresare v nadrazenem adresari
 * @param filesystem_data filesystem
 * @param fs_file otevreny soubor filesystemu
 * @param inode adresar
 * @param parrent nadadresar
 */
void
removeLinkInParrentDir(filesystem &filesystem_data, std::fstream &fs_file, pseudo_inode &inode, pseudo_inode &parrent) {
    // adresare v datablocku
    uint32_t dirs_per_cluster = filesystem_data.super_block.cluster_size / sizeof(directory_item);
    directory_item directories[dirs_per_cluster];

    // platne adresy na databloky
    std::vector<int32_t> addresses = usedDatablockByINode(filesystem_data, fs_file, parrent, false);

    // prochazeni adres databloku
    for (auto &address : addresses) {
        // odstraneni referenci
        fs_file.seekp(address);
        fs_file.read(reinterpret_cast<char *>(&directories), sizeof(directories));
        for (int i = 0; i < dirs_per_cluster; i++) {
            if (directories[i].inode == inode.nodeid) {
                directories[i].inode = 0;
                fs_file.seekp(address);
                fs_file.write(reinterpret_cast<const char *>(&directories), sizeof(directories));
            }
        }
    }
}

/**
 * Uvolni v bitmape datablok
 * @param filesystem_data filesystem
 * @param fs_file otevreny soubor filesystemu
 * @param datablock_position pozice databloku
 */
void removeDatablockPositionInBitmap(filesystem &filesystem_data, std::fstream &fs_file, int32_t datablock_position) {

    // urceni pozice v bitmape
    int32_t datablock_no = (datablock_position - filesystem_data.super_block.data_start_address) /
                           filesystem_data.super_block.cluster_size;
    int32_t byte_no = datablock_no / 8;
    int32_t bite_no = datablock_no % 8;
    //std::cout << "DTB NO: " << datablock_no << " BYTE NO: " << byte_no << " BITE NO: " << bite_no << std::endl;

    // zmena bitu v byte
    char b;
    uint8_t bitmap_byte;
    fs_file.seekp(filesystem_data.super_block.bitmap_start_address + byte_no);
    fs_file.read(&b, 1);
    std::bitset<8> x(b);
    x.set(bite_no, false);
    unsigned long ul = x.to_ulong();
    auto c = static_cast<unsigned int>(ul);
    bitmap_byte = c;

    // zapis upravene bitmapy
    fs_file.seekp(filesystem_data.super_block.bitmap_start_address + byte_no);
    fs_file.write(reinterpret_cast<const char *>(&bitmap_byte), 1);
}

/**
 * Uvolni v bitmape databloky souboru
 * @param filesystem_data filesystem
 * @param fs_file otevreny soubor filesystemu
 * @param inode soubor
 */
void removeDatablocksPositionInBitmap(filesystem &filesystem_data, std::fstream &fs_file, pseudo_inode &inode) {
    // platne adresy na databloky
    std::vector<int32_t> addresses = usedDatablockByINode(filesystem_data, fs_file, inode, false);

    // prochazeni adres databloku
    for (auto &address : addresses) {
        // odstraneni databloku
        removeDatablockPositionInBitmap(filesystem_data, fs_file, address);
    }

    // odstraneni databloku pro neprime odkazy
    if (inode.indirect1 != 0) {
        removeDatablockPositionInBitmap(filesystem_data, fs_file, inode.indirect1);
    }
    if (inode.indirect2 != 0) {
        uint32_t links_per_cluster = filesystem_data.super_block.cluster_size / sizeof(int32_t);
        int32_t links[links_per_cluster];
        fs_file.seekp(inode.indirect2);
        fs_file.read(reinterpret_cast<char *>(&links), sizeof(links));
        for (int i = 0; i < links_per_cluster; i++) {
            if (links[i] != 0) {
                removeDatablockPositionInBitmap(filesystem_data, fs_file, links[i]);
            }
        }
        removeDatablockPositionInBitmap(filesystem_data, fs_file, inode.indirect2);
    }
}

/**
 * Smaze prazdny adresar a1
 * @param filesystem_data fileszystem
 * @param a1 nazev adresare
 */
void rmdir(filesystem &filesystem_data, std::string &a1) {

    // nalezeni adresare a1
    pseudo_inode a1_inode;
    pseudo_inode *a1_inode_ptr = iNodeByLocation(filesystem_data, a1, false, false);
    if (a1_inode_ptr != nullptr) {
        a1_inode = *a1_inode_ptr;
        if (a1_inode.type != 1) {
            std::cout << "FILE IS NOT DIRECTORY" << std::endl;
            return;
        }
        if (a1_inode.nodeid == 0) {
            std::cout << "INVALID FILE" << std::endl;
            return;
        }
    } else {
        std::cout << "FILE NOT FOUND" << std::endl;  // FILE FILE DOES NOT EXISTS
        return;
    }


    std::fstream fs_file;
    fs_file.open(filesystem_data.fs_file, std::ios::in | std::ios::out | std::ios::binary);

    bool empty = isDirectoryEmpty(filesystem_data, fs_file, a1_inode);
    if (empty) {
        pseudo_inode *parrent_ptr = getParrentDirectory(filesystem_data, fs_file, a1_inode);
        pseudo_inode parrent;
        if (parrent_ptr != nullptr) {
            parrent = *parrent_ptr;
            if (parrent.nodeid != a1_inode.nodeid) {
                removeLinkInParrentDir(filesystem_data, fs_file, a1_inode, parrent);
                removeDatablockPositionInBitmap(filesystem_data, fs_file, a1_inode.direct1);
                removeINode(filesystem_data, fs_file, a1_inode);
                std::cout << "OK" << std::endl;
            } else {
                std::cout << "ROOT CANNOT BE REMOVED" << std::endl;
            }
        }
    } else {
        std::cout << "NOT EMPTY" << std::endl;
    }

    fs_file.close();
}

/**
 * Smaze soubor s1
 * @param filesystem_data fileszystem
 * @param s1 nazev souboru
 */
void rm(filesystem &filesystem_data, std::string &s1) {

    // nalezeni nadrazeneho adresare
    std::vector<std::string> s1_segments = splitPath(s1);
    pseudo_inode a1_inode;
    pseudo_inode *a1_inode_ptr = cd(filesystem_data, s1, false, false);
    if (a1_inode_ptr != nullptr) {
        a1_inode = *a1_inode_ptr;
    } else {
        std::cout << "FILE NOT FOUND" << std::endl;  // SOURCE DIRECTORY DOES NOT EXISTS
        return;
    }

    // nalezeni souboru s1
    pseudo_inode s1_inode;
    pseudo_inode *s1_inode_ptr = iNodeByLocation(filesystem_data, s1, false, false);
    if (s1_inode_ptr != nullptr) {
        s1_inode = *s1_inode_ptr;
        if (s1_inode.type == 1) {
            std::cout << "FILE IS DIRECTORY" << std::endl;
            return;
        }
        if (s1_inode.nodeid == 0) {
            std::cout << "INVALID FILE" << std::endl;
            return;
        }
    } else {
        std::cout << "FILE NOT FOUND" << std::endl;  // FILE FILE DOES NOT EXISTS
        return;
    }

    // nalezeni zaznamu souboru s1 v adresari a1
    directory_item s1_dir;
    directory_item *s1_dir_ptr = nullptr;
    s1_dir_ptr = getDirectoryItem(filesystem_data, a1_inode, s1_segments.back());
    if (s1_dir_ptr != nullptr) {
        s1_dir = *s1_dir_ptr;
    } else {
        std::cout << "FILE NOT FOUND" << std::endl;   // SOURCE DIRECTORY ITEM DOES NOT EXISTS
        return;
    }

    std::fstream fs_file;
    fs_file.open(filesystem_data.fs_file, std::ios::in | std::ios::out | std::ios::binary);

    fs_file.seekp(getINodePosition(filesystem_data, s1_dir.inode));
    fs_file.read(reinterpret_cast<char *>(&s1_inode), sizeof(pseudo_inode));

    // smazani souboru s1
    removeLinkInParrentDir(filesystem_data, fs_file, s1_inode, a1_inode);
    removeDatablocksPositionInBitmap(filesystem_data, fs_file, s1_inode);
    removeINode(filesystem_data, fs_file, s1_inode);

    fs_file.close();

    std::cout << "OK" << std::endl;
}