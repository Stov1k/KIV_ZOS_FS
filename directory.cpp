//
// Created by pavel on 28.12.19.
//

#include <cstring>
#include <bitset>
#include <vector>
#include <iostream>
#include <fstream>
#include "zosfsstruct.h"

/**
 * Vypise adresare
 * @param directories vektor adresaru
 */
void printDirectories(const std::vector<directory_item>& directories) {
    for(auto& directory : directories) {
        std::cout << "\t"<< directory.inode << "\t" << directory.item_name << std::endl;
    }
}

/**
 * Vrati vektor podadresaru v aktualnim adresari
 * @return vektor adresaru
 */
std::vector<directory_item> getDirectories(filesystem &filesystem_data) {
    std::fstream input_file;
    input_file.open(filesystem_data.fs_file, std::ios::in);

    std::vector<directory_item> directories;

    // pocet adresaru na jeden data blok
    uint32_t dirs_per_cluster = filesystem_data.super_block.cluster_size / sizeof(directory_item);

    // skok na prvni primy odkaz v inode (TODO: pozdeji predelat na iteraci)
    input_file.seekp(filesystem_data.current_dir.direct1);

    // nacteni slozek a vlozeni do vectoru
    directory_item dirs_array[dirs_per_cluster];
    input_file.read(reinterpret_cast<char *>(&dirs_array), sizeof(dirs_array));
    for(int i = 0; i < dirs_per_cluster; i++) {
        if (dirs_array[i].inode) {
            directories.push_back(dirs_array[i]);
        }
    }
    input_file.close();
    return directories;
}

/**
 * Vrati, zdali adresar tehoz jmena jiz existuje
 * @param dir adresar
 * @return existuje adresar stejneho jmena?
 */
bool isDirectoryExists(filesystem& filesystem_data, directory_item& dir) {
    std::vector<directory_item> directories = getDirectories(filesystem_data);
    for(auto& directory : directories) {
        directory.item_name;
        if(strcmp(dir.item_name,directory.item_name) == 0) {
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