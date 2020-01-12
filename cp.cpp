//
// Created by pavel on 12.01.20.
//

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include "zosfsstruct.h"
#include "directory.h"
#include "inode.h"
#include "cd.h"
#include "datablock.h"
#include "cp.h"

/**
 * Zkopireje zdrojovy datablok do ciloveho databloku
 * @param filesystem_data informace filesystemu
 * @param fs_file otevreny soubor na pseudoNTFS
 * @param source_address pocatecni pozice zdrojoveho datablocku
 * @param target_address pocatecni pozice ciloveho datablocku
 */
void copyDataBlock(filesystem &filesystem_data, std::fstream &fs_file, int32_t source_address, int32_t target_address) {
    int32_t block_size = filesystem_data.super_block.cluster_size;
    // priprava bufferu
    char buffer[block_size];
    memset(buffer, EOF, block_size);
    // presun na pozici zdroje
    fs_file.seekp(source_address);
    fs_file.read(buffer, block_size);
    // presun na pozici cile
    fs_file.seekp(target_address);
    fs_file.write(buffer, block_size);
}

/**
 * Zkopiruje soubor s1 do umisteni s2
 * @param filesystem_data filesystem
 * @param s1 nazev souboru
 * @param s2 nazev souboru
 */
void cp(filesystem &filesystem_data, std::string &s1, std::string &s2) {

    // cesta rozdelena na adresare
    std::vector<std::string> from_segments = splitPath(s1);
    std::vector<std::string> to_segments = splitPath(s2);

    // pracovni adresare
    pseudo_inode *from_dir_ptr = cd(filesystem_data, s1, false, false);
    pseudo_inode from_dir = filesystem_data.current_dir;
    if(from_dir_ptr != nullptr) {
        from_dir = *from_dir_ptr;
    } else {
        std::cout << "FILE NOT FOUND" << std::endl;  // SOURCE DIRECTORY DOES NOT EXISTS
        return;
    }

    pseudo_inode *to_dir_ptr = cd(filesystem_data, s2, false, false);
    pseudo_inode to_dir = filesystem_data.current_dir;
    if(to_dir_ptr != nullptr) {
        to_dir = *to_dir_ptr;
    } else {
        std::cout << "PATH NOT FOUND" << std::endl; // DESTINATION DIRECTORY DOES NOT EXISTS
        return;
    }

    // najdu odpovidajici inode zdroje
    pseudo_inode source_inode;
    pseudo_inode *source_inode_ptr = getFileINode(filesystem_data, from_dir, from_segments.back(), false);
    if (source_inode_ptr != nullptr) {
        source_inode = *source_inode_ptr;
    } else {
        std::cout << "FILE NOT FOUND" << std::endl;   // SOURCE FILE DOES NOT EXISTS
        return;
    }

    // platne adresy na databloky
    std::fstream fs_file;
    fs_file.open(filesystem_data.fs_file, std::ios::in | std::ios::out | std::ios::binary);
    std::vector<int32_t> source_addresses = usedDatablockByINode(filesystem_data, fs_file, source_inode, true);
    int32_t blocks_used = source_addresses.size();

    // dostupnych volnych databloku
    int32_t blocks_available = availableDatablocks(filesystem_data);

    if(blocks_available < blocks_used) {
        std::cout << "NOT ENOUGH SPACE!" << std::endl;
        return;
    }

    // najdu volny inode
    pseudo_inode target_inode;
    pseudo_inode *target_inode_ptr;
    target_inode_ptr = getFreeINode(filesystem_data);
    if (target_inode_ptr != nullptr) {
        target_inode = *target_inode_ptr;
        target_inode.isDirectory = source_inode.isDirectory;
        target_inode.file_size = source_inode.file_size;
        fs_file.seekp(getINodePosition(filesystem_data, target_inode.nodeid));
        fs_file.write(reinterpret_cast<const char *>(&target_inode), sizeof(pseudo_inode));
        directory_item target_dir = createDirectoryItem(target_inode.nodeid, to_segments.back());
        int32_t address = addDirectoryItemEntry(filesystem_data, to_dir, target_dir);
    } else {
        std::cout << "NO FREE INODE LEFT!" << std::endl;
        return;
    }

    // platne adresy na databloky
    source_addresses = usedDatablockByINode(filesystem_data, fs_file, source_inode, false);
    std::cout << "COPYING";
    ino_t progress = 0;
    // prochazeni adres databloku
    for (auto &address : source_addresses) {
        if(!progress%1024) {
            std::cout << ".";
        }
        int32_t obtained_address = addDatablockToINode(filesystem_data, fs_file, target_inode);
        copyDataBlock(filesystem_data, fs_file, address, obtained_address);
        progress++;
    }
    std::cout << std::endl;

    fs_file.close();
    std::cout << "OK" << std::endl;
}
