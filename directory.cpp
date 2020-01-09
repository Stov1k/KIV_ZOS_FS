//
// Created by pavel on 28.12.19.
//

#include <cstring>
#include <bitset>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include "zosfsstruct.h"
#include "inode.h"
#include "datablock.h"

/**
 * Pocet adresaru, ktere lze ulozit do jednoho databloku
 * @param filesystem_data filesystem
 * @return pocet adresaru
 */
int32_t dirsPerCluster(filesystem &filesystem_data) {
    return filesystem_data.super_block.cluster_size / sizeof(directory_item);
}

/**
 * Rozdeleni cesty na adresare
 * @param path cesta
 * @return segments vektor adresaru
 */
std::vector<std::string> splitPath(const std::string path) {
    std::stringstream full_path(path);
    std::string segment;
    std::vector<std::string> segments;

    while (std::getline(full_path, segment, '/')) {
        segments.push_back(segment);
    }
    return segments;
}

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
 * @param filesystem_data filesystem
 * @param working_dir pracovni adresar
 * @return vektor adresaru
 */
std::vector<directory_item> getDirectories(filesystem &filesystem_data, pseudo_inode &working_dir) {
    std::fstream fs_file;
    fs_file.open(filesystem_data.fs_file, std::ios::in);

    std::vector<directory_item> directories;

    // pocet adresaru na jeden data blok
    int32_t dirs_per_cluster = dirsPerCluster(filesystem_data);
    directory_item dirs_array[dirs_per_cluster];

    // platne adresy na databloky
    std::vector<int32_t> addresses = usedDatablockByINode(filesystem_data,fs_file, working_dir);

    // prochazeni adres databloku
    for (auto &address : addresses) {
        // nacteni slozek a vlozeni do vectoru
        fs_file.seekp(address);
        fs_file.read(reinterpret_cast<char *>(&dirs_array), sizeof(dirs_array));
        for (int i = 0; i < dirs_per_cluster; i++) {
            if (dirs_array[i].inode) {
                directories.push_back(dirs_array[i]);
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
    std::vector<directory_item> directories = getDirectories(filesystem_data, filesystem_data.current_dir);
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