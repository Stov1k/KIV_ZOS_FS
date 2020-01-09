//
// Created by pavel on 09.01.20.
//

#include <iostream>
#include <fstream>
#include <vector>
#include "ls.h"
#include "zosfsstruct.h"
#include "inode.h"
#include "datablock.h"
#include "directory.h"

/**
 * Vypise obsah adresare a1
 * @param filesystem_data filesystem
 * @param a1 adresar
 */
void ls(filesystem &filesystem_data, pseudo_inode &a1) {
    std::fstream fs_file;
    fs_file.open(filesystem_data.fs_file, std::ios::in);

    // platne adresy na soubory a adresare
    if (fs_file.is_open()) {
        // platne adresy na soubory a adresare
        std::vector<int32_t> addresses;
        if (a1.direct1) addresses.push_back(a1.direct1);
        if (a1.direct2) addresses.push_back(a1.direct2);
        if (a1.direct3) addresses.push_back(a1.direct3);
        if (a1.direct4) addresses.push_back(a1.direct4);
        if (a1.direct5) addresses.push_back(a1.direct5);
        if (a1.indirect1 || a1.indirect1) {
            int32_t links_per_cluster = linksPerCluster(filesystem_data);
            int32_t links[links_per_cluster];
            if (a1.indirect1) {
                fs_file.seekp(a1.indirect1);
                fs_file.read(reinterpret_cast<char *>(&links), sizeof(links));
                for (int i = 0; i < links_per_cluster; i++) {
                    if (links[i]) addresses.push_back(links[i]);
                }
            }
            if (a1.indirect2) {
                int32_t sublinks[links_per_cluster];
                fs_file.seekp(a1.indirect2);
                fs_file.read(reinterpret_cast<char *>(&links), sizeof(links));
                for (int i = 0; i < links_per_cluster; i++) {
                    if (links[i]) {
                        fs_file.seekp(links[i]);
                        fs_file.read(reinterpret_cast<char *>(&sublinks), sizeof(sublinks));
                        for (int j = 0; j < links_per_cluster; j++) {
                            if (sublinks[j]) addresses.push_back(sublinks[j]);
                        }
                    };
                }
            }
        }
        // adresare databloku
        uint32_t dirs_per_cluster = dirsPerCluster(filesystem_data);
        directory_item directories[dirs_per_cluster];

        // vypise hlavicku
        std::cout << "#" << "\t" << "ND" << "\t" << "NAME" << "\t" << "SIZE" << std::endl;

        // prochazeni adres databloku
        for (auto &address : addresses) {
            fs_file.seekp(address);     // prejde na adresu databloku
            fs_file.read(reinterpret_cast<char *>(&directories), sizeof(directories));
            for (int i = 0; i < dirs_per_cluster; i++) {
                if (directories[i].inode) {
                    pseudo_inode inode;
                    fs_file.seekp(getINodePosition(filesystem_data, directories[i].inode));
                    fs_file.read(reinterpret_cast<char *>(&inode), sizeof(pseudo_inode));
                    std::cout << i << "\t" << directories[i].inode << "\t" << directories[i].item_name << "\t"
                              << inode.file_size << std::endl;
                }
            }
        }
    }

    fs_file.close();
}