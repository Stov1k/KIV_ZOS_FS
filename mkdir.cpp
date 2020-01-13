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

    // nalezeni nadrazeneho adresare
    std::vector<std::string> a1_segments = splitPath(a1);
    pseudo_inode a1_parrent_inode;
    pseudo_inode *a1_parrent_inode_ptr = cd(filesystem_data, a1, false, false);
    if (a1_parrent_inode_ptr != nullptr) {
        a1_parrent_inode = *a1_parrent_inode_ptr;
    } else {
        std::cout << "PATH NOT FOUND" << std::endl;  // PARRENT DIRECTORY DOES NOT EXISTS
        return;
    }
    // adresar jiz existuje
    if (nullptr != iNodeByLocation(filesystem_data, a1, false)) {
        std::cout << "EXIST" << std::endl;
        return;
    }
    // pocet dostupnych volnych databloku
    int32_t blocks_available = availableDatablocks(filesystem_data);
    if (!blocks_available) {
        std::cout << "NOT ENOUGH SPACE" << std::endl;
        return;
    }
    // nalezeny volny inode
    pseudo_inode a1_inode;
    pseudo_inode *a1_inode_ptr = getFreeINode(filesystem_data);
    if (a1_inode_ptr != nullptr) {
        a1_inode = *a1_inode_ptr;
        a1_inode.type = 1;
        a1_inode.file_size = filesystem_data.super_block.cluster_size;
    } else {
        std::cout << "NO FREE INODE LEFT" << std::endl;
        return;
    }


    std::fstream fs_file;
    fs_file.open(filesystem_data.fs_file, std::ios::in | std::ios::out | std::ios::binary);

    // zapis inode
    fs_file.seekp(getINodePosition(filesystem_data, a1_inode.nodeid));
    fs_file.write(reinterpret_cast<const char *>(&a1_inode), sizeof(pseudo_inode));

    // zapis zaznamu adresare v nadrazenem adresari
    directory_item target_dir = createDirectoryItem(a1_inode.nodeid, a1_segments.back());
    int32_t address = addDirectoryItemEntry(filesystem_data, a1_parrent_inode, target_dir);
    if (!address) {
        std::cout << "MAXIMUM NUMBER OF FILES REACHED" << std::endl;
        removeINode(filesystem_data, fs_file, a1_inode);
        fs_file.close();
        return;
    }

    // pridam datablok
    int32_t obtained_address = addDatablockToINode(filesystem_data, fs_file, a1_inode);

    // aktualni adresar
    directory_item dir_dot;
    dir_dot = createDirectoryItem(a1_inode.nodeid, ".");

    // predchozi adresar
    directory_item dir_parrent;
    dir_parrent = createDirectoryItem(a1_parrent_inode.nodeid, "..");

    int32_t dirs_per_cluster = dirsPerCluster(filesystem_data);
    directory_item subdirectories[dirs_per_cluster];
    subdirectories[0] = dir_dot;                    // .
    subdirectories[1] = dir_parrent;                // ..
    for (int i = 2; i < dirs_per_cluster; i++) {    // dalsi neobsazene
        subdirectories[i] = directory_item{};
        subdirectories[i].inode = 0;   // tj. nic
    }

    // aktualizace podslozek v novem adresari
    fs_file.seekp(a1_inode.direct1);
    fs_file.write(reinterpret_cast<const char *>(&subdirectories), sizeof(subdirectories));

    fs_file.close();

    std::cout << "OK" << std::endl;
}