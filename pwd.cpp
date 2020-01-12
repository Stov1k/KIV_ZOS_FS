//
// Created by pavel on 06.01.20.
//

#include <stack>
#include <fstream>
#include <iostream>
#include <sstream>
#include "pwd.h"
#include "zosfsstruct.h"
#include "inode.h"
#include "directory.h"
#include "datablock.h"

/**
 * Vrati nazev adresare v nadrazenem adresari
 * @param filesystem_data filesystem
 * @param fs_file otevreny soubor filesystemu
 * @param inode adresar
 * @param parrent nadadresar
 */
std::string
getDirectoryName(filesystem &filesystem_data, std::fstream &fs_file, pseudo_inode &inode, pseudo_inode &parrent) {
    // adresare v datablocku
    uint32_t dirs_per_cluster = filesystem_data.super_block.cluster_size / sizeof(directory_item);
    directory_item directories[dirs_per_cluster];

    // platne adresy na databloky
    std::vector<int32_t> addresses = usedDatablockByINode(filesystem_data, fs_file, parrent, false);

    // prochazeni adres databloku
    for (auto &address : addresses) {
        fs_file.seekp(address);
        fs_file.read(reinterpret_cast<char *>(&directories), sizeof(directories));
        for (int i = 0; i < dirs_per_cluster; i++) {
            if (directories[i].inode == inode.nodeid) {
                return directories[i].item_name;
            }
        }
    }
}

/**
 * Vrati cestu jako string
 * @param filesystem_data filesystem
 * @param fs_file otevreny soubor filesystemu
 * @param s zasobnik
 */
std::string getPathString(filesystem &filesystem_data, std::fstream &fs_file, std::stack<pseudo_inode> s) {
    std::stringstream buffer;
    pseudo_inode prev = s.top();
    s.pop();
    while (!s.empty()) {
        pseudo_inode top = s.top();
        buffer << "/" << getDirectoryName(filesystem_data, fs_file, top, prev);
        prev = top;
        s.pop();
    }
    return buffer.str();
}

/**
 * Vypise cestu
 * @param filesystem_data filesystem
 * @param inode adresar
 * @param verbose vypise cestu
 * @return cesta
 */
std::string pwd(filesystem &filesystem_data, pseudo_inode &inode, bool verbose) {
    std::string path = "";

    std::fstream fs_file;
    fs_file.open(filesystem_data.fs_file, std::ios::in | std::ios::out | std::ios::binary);

    std::stack<pseudo_inode> s_path;

    pseudo_inode current = inode;
    pseudo_inode root = filesystem_data.root_dir;

    if (current.isDirectory) {
        s_path.push(current);
        do {
            current = *getParrentDirectory(filesystem_data, fs_file, current);
            s_path.push(current);
        } while (current.nodeid != root.nodeid);

        if (verbose) {
            path = getPathString(filesystem_data, fs_file, s_path);
            std::cout << path << '\n';
        }
    } else {
        if (verbose) {
            std::cout << "INODE " << inode.nodeid << " IS NOT DIRECTORY" << std::endl;
        }
    }

    fs_file.close();

    return path;
}

/**
 * Vypise aktualni cestu
 * @param filesystem_data filesystem
 * @return cesta
 */
std::string pwd(filesystem &filesystem_data) {
    return pwd(filesystem_data, filesystem_data.current_dir, true);
}