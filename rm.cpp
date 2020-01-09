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
    std::vector<int32_t> addresses = usedDatablockByINode(filesystem_data, fs_file, inode);

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
    std::vector<int32_t> addresses = usedDatablockByINode(filesystem_data, fs_file, parrent);

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
    std::vector<int32_t> addresses = usedDatablockByINode(filesystem_data, fs_file, inode);

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
 * Uvolni inode
 * @param filesystem_data filesystem
 * @param fs_file otevreny soubor filesystemu
 * @param inode adresar
 */
void removeINode(filesystem &filesystem_data, std::fstream &fs_file, pseudo_inode &inode) {
    int32_t inode_position = getINodePosition(filesystem_data, inode.nodeid);
    inode.references = inode.references - 1;
    if (inode.references < 1) {
        inode.direct1 = 0;
        inode.direct2 = 0;
        inode.direct3 = 0;
        inode.direct4 = 0;
        inode.direct5 = 0;
        inode.indirect1 = 0;
        inode.indirect2 = 0;
        inode.file_size = 0;
        inode.nodeid = 0;
    }
    fs_file.seekp(inode_position);
    fs_file.write(reinterpret_cast<const char *>(&inode), sizeof(pseudo_inode));
}

/**
 * Smaze prazdny adresar a1
 * @param filesystem_data fileszystem
 * @param a1 nazev adresare
 */
void rmdir(filesystem &filesystem_data, std::string &a1) {
    std::fstream fs_file;
    fs_file.open(filesystem_data.fs_file, std::ios::in | std::ios::out | std::ios::binary);

    pseudo_inode *inode_ptr = nullptr;
    pseudo_inode inode;

    // zjistim, zdali existuje adresar stejneho nazvu
    std::vector<directory_item> directories = getDirectories(filesystem_data, filesystem_data.current_dir);
    for (auto &directory : directories) {
        if (strcmp(a1.c_str(), directory.item_name) == 0) {
            fs_file.seekp(
                    filesystem_data.super_block.inode_start_address + (directory.inode - 1) * sizeof(pseudo_inode));
            fs_file.read(reinterpret_cast<char *>(&inode), sizeof(pseudo_inode));
            if (inode.isDirectory) {
                inode_ptr = &inode;
            }
            break;
        }
    }

    if (inode_ptr != nullptr) {
        inode = *inode_ptr;     // TODO: asi uz nastavene, mozno smazat
        bool empty = isDirectoryEmpty(filesystem_data, fs_file, inode);
        if (empty) {
            pseudo_inode *parrent_ptr = getParrentDirectory(filesystem_data, fs_file, inode);
            pseudo_inode parrent;
            if (parrent_ptr != nullptr) {
                parrent = *parrent_ptr;
                if (parrent.nodeid != inode.nodeid) {
                    removeLinkInParrentDir(filesystem_data, fs_file, inode, parrent);
                    removeDatablockPositionInBitmap(filesystem_data, fs_file, inode.direct1);
                    removeINode(filesystem_data, fs_file, inode);
                    std::cout << "OK" << std::endl;
                } else {
                    std::cout << "ROOT CANNOT BE REMOVED" << std::endl;
                }
            }
        } else {
            std::cout << "NOT EMPTY" << std::endl;
        }
    } else {
        std::cout << "FILE NOT FOUND" << std::endl;
    }

    fs_file.close();
}

/**
 * Smaze soubor s1
 * @param filesystem_data fileszystem
 * @param s1 nazev souboru
 */
void rm(filesystem &filesystem_data, std::string &a1) {
    std::fstream fs_file;
    fs_file.open(filesystem_data.fs_file, std::ios::in | std::ios::out | std::ios::binary);

    pseudo_inode *inode_ptr = nullptr;
    pseudo_inode inode;

    // zjistim, zdali existuje soubor stejneho nazvu
    std::vector<directory_item> directories = getDirectories(filesystem_data, filesystem_data.current_dir);
    for (auto &directory : directories) {
        if (strcmp(a1.c_str(), directory.item_name) == 0) {
            fs_file.seekp(
                    filesystem_data.super_block.inode_start_address + (directory.inode - 1) * sizeof(pseudo_inode));
            fs_file.read(reinterpret_cast<char *>(&inode), sizeof(pseudo_inode));
            if (!inode.isDirectory) {
                inode_ptr = &inode;
            }
            break;
        }
    }

    if (inode_ptr != nullptr) {
        inode = *inode_ptr;     // TODO: asi uz nastavene, mozno smazat
        removeLinkInParrentDir(filesystem_data, fs_file, inode, filesystem_data.current_dir);
        removeDatablocksPositionInBitmap(filesystem_data, fs_file, inode);
        removeINode(filesystem_data, fs_file, inode);
        std::cout << "OK" << std::endl;

    } else {
        std::cout << "FILE NOT FOUND" << std::endl;
    }

    fs_file.close();
}