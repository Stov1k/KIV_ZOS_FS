//
// Created by pavel on 28.12.19.
//

#include <cstring>
#include <bitset>
#include <vector>
#include <iostream>
#include <fstream>
#include "zosfsstruct.h"
#include "inode.h"

/**
 * Vypise adresare
 * @param directories vektor adresaru
 */
void printDirectories(const std::vector<directory_item> &directories) {
    for (auto &directory : directories) {
        std::cout << "\t" << directory.inode << "\t" << directory.item_name << std::endl;
    }
}

/**
 * Vrati vektor podadresaru v aktualnim adresari
 * @return vektor adresaru
 */
std::vector<directory_item> getDirectories(filesystem &filesystem_data) {
    std::fstream fs_file;
    fs_file.open(filesystem_data.fs_file, std::ios::in);

    std::vector<directory_item> directories;

    // pocet adresaru na jeden data blok
    uint32_t dirs_per_cluster = filesystem_data.super_block.cluster_size / sizeof(directory_item);
    directory_item dirs_array[dirs_per_cluster];

    // nacteni slozek a vlozeni do vectoru
    if (filesystem_data.current_dir.direct1 != 0) {
        fs_file.seekp(filesystem_data.current_dir.direct1);
        fs_file.read(reinterpret_cast<char *>(&dirs_array), sizeof(dirs_array));
        for (int i = 0; i < dirs_per_cluster; i++) {
            if (dirs_array[i].inode) {
                directories.push_back(dirs_array[i]);
            }
        }
    }
    if (filesystem_data.current_dir.direct2 != 0) {
        fs_file.seekp(filesystem_data.current_dir.direct2);

        fs_file.read(reinterpret_cast<char *>(&dirs_array), sizeof(dirs_array));
        for (int i = 0; i < dirs_per_cluster; i++) {
            if (dirs_array[i].inode) {
                directories.push_back(dirs_array[i]);
            }
        }
    }
    if (filesystem_data.current_dir.direct3 != 0) {
        fs_file.seekp(filesystem_data.current_dir.direct3);

        fs_file.read(reinterpret_cast<char *>(&dirs_array), sizeof(dirs_array));
        for (int i = 0; i < dirs_per_cluster; i++) {
            if (dirs_array[i].inode) {
                directories.push_back(dirs_array[i]);
            }
        }
    }
    if (filesystem_data.current_dir.direct4 != 0) {
        fs_file.seekp(filesystem_data.current_dir.direct4);
        fs_file.read(reinterpret_cast<char *>(&dirs_array), sizeof(dirs_array));
        for (int i = 0; i < dirs_per_cluster; i++) {
            if (dirs_array[i].inode) {
                directories.push_back(dirs_array[i]);
            }
        }
    }
    if (filesystem_data.current_dir.direct5 != 0) {
        fs_file.seekp(filesystem_data.current_dir.direct5);
        fs_file.read(reinterpret_cast<char *>(&dirs_array), sizeof(dirs_array));
        for (int i = 0; i < dirs_per_cluster; i++) {
            if (dirs_array[i].inode) {
                directories.push_back(dirs_array[i]);
            }
        }
    }
    if (filesystem_data.current_dir.indirect1 != 0) {
        uint32_t links_per_cluster = filesystem_data.super_block.cluster_size / sizeof(int32_t);
        int32_t links[links_per_cluster];
        fs_file.seekp(filesystem_data.current_dir.indirect1);
        fs_file.read(reinterpret_cast<char *>(&links), sizeof(links));
        for (int i = 0; i < links_per_cluster; i++) {
            if (links[i] != 0) {
                fs_file.seekp(links[i]);
                fs_file.read(reinterpret_cast<char *>(&dirs_array), sizeof(dirs_array));
                for (int i = 0; i < dirs_per_cluster; i++) {
                    if (dirs_array[i].inode) {
                        directories.push_back(dirs_array[i]);
                    }
                }
            }
        }
    }
    if (filesystem_data.current_dir.indirect2 != 0) {
        uint32_t links_per_cluster = filesystem_data.super_block.cluster_size / sizeof(int32_t);
        int32_t links[links_per_cluster];
        fs_file.seekp(filesystem_data.current_dir.indirect2);
        fs_file.read(reinterpret_cast<char *>(&links), sizeof(links));
        for (int i = 0; i < links_per_cluster; i++) {
            if (links[i] != 0) {
                int32_t sublinks[links_per_cluster];
                fs_file.seekp(links[i]);
                fs_file.read(reinterpret_cast<char *>(&sublinks), sizeof(sublinks));
                for (int j = 0; j < links_per_cluster; j++) {
                    if (sublinks[j] != 0) {
                        fs_file.seekp(sublinks[j]);
                        fs_file.read(reinterpret_cast<char *>(&dirs_array), sizeof(dirs_array));
                        for (int i = 0; i < dirs_per_cluster; i++) {
                            if (dirs_array[i].inode) {
                                directories.push_back(dirs_array[i]);
                            }
                        }
                    }
                }
            }
        }
    }

    fs_file.close();
    return directories;
}

/**
 * Vrati, zdali adresar tehoz jmena jiz existuje
 * @param dir adresar
 * @return existuje adresar stejneho jmena?
 */
bool isDirectoryExists(filesystem &filesystem_data, directory_item &dir) {
    std::vector<directory_item> directories = getDirectories(filesystem_data);
    for (auto &directory : directories) {
        directory.item_name;
        if (strcmp(dir.item_name, directory.item_name) == 0) {
            return true;
        }
    }
    return false;
}

/**
 * Vrati referenci na adresar
 * @param nodeid cislo node
 * @param name nazev adresare
 * @return reference na adresar
 */
directory_item getDirectory(int32_t nodeid, std::string name) {
    directory_item dir = directory_item{};
    dir.inode = nodeid;
    std::strncpy(dir.item_name, name.c_str(), sizeof(dir.item_name));
    dir.item_name[sizeof(dir.item_name) - 1] = '\0';
    return dir;
}

/**
 * Vrati nadrazeny adresar
 * @param filesystem_data filesystem
 * @param fs_file otevreny soubor filesystemu
 * @param inode adresar
 * @return nadrazeny adresar
 */
pseudo_inode *getParrentDirectory(filesystem &filesystem_data, std::fstream &fs_file, pseudo_inode &inode) {

    uint32_t dirs_per_cluster = filesystem_data.super_block.cluster_size / sizeof(directory_item);
    directory_item directories[dirs_per_cluster];

    pseudo_inode parrent;
    pseudo_inode *parrent_ptr = nullptr;

    if (inode.direct1 != 0) {
        fs_file.seekp(inode.direct1);
        fs_file.read(reinterpret_cast<char *>(&directories), sizeof(directories));
        fs_file.seekp(getINodePosition(filesystem_data, directories[1].inode));
        fs_file.read(reinterpret_cast<char *>(&parrent), sizeof(parrent));
        parrent_ptr = &parrent;
    }

    return parrent_ptr;
}