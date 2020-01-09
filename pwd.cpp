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

/**
 * Vrati nazev adresare v nadrazenem adresari
 * @param filesystem_data filesystem
 * @param fs_file otevreny soubor filesystemu
 * @param inode adresar
 * @param parrent nadadresar
 */
std::string getDirectoryName(filesystem &filesystem_data, std::fstream &fs_file, pseudo_inode &inode, pseudo_inode &parrent) {
    // adresare v datablocku
    uint32_t dirs_per_cluster = filesystem_data.super_block.cluster_size / sizeof(directory_item);
    directory_item directories[dirs_per_cluster];

    if (parrent.direct1 != 0) {
        fs_file.seekp(parrent.direct1);
        fs_file.read(reinterpret_cast<char *>(&directories), sizeof(directories));
        for (int i = 0; i < dirs_per_cluster; i++) {
            if (directories[i].inode == inode.nodeid) {
                return directories[i].item_name;
            }
        }
    }
    if (parrent.direct2 != 0) {
        fs_file.seekp(parrent.direct2);
        fs_file.read(reinterpret_cast<char *>(&directories), sizeof(directories));
        for (int i = 0; i < dirs_per_cluster; i++) {
            if (directories[i].inode == inode.nodeid) {
                return directories[i].item_name;
            }
        }
    }
    if (parrent.direct3 != 0) {
        fs_file.seekp(parrent.direct3);
        fs_file.read(reinterpret_cast<char *>(&directories), sizeof(directories));
        for (int i = 0; i < dirs_per_cluster; i++) {
            if (directories[i].inode == inode.nodeid) {
                return directories[i].item_name;
            }
        }
    }
    if (parrent.direct4 != 0) {
        fs_file.seekp(parrent.direct4);
        fs_file.read(reinterpret_cast<char *>(&directories), sizeof(directories));
        for (int i = 0; i < dirs_per_cluster; i++) {
            if (directories[i].inode == inode.nodeid) {
                return directories[i].item_name;
            }
        }
    }
    if (parrent.direct5 != 0) {
        fs_file.seekp(parrent.direct5);
        fs_file.read(reinterpret_cast<char *>(&directories), sizeof(directories));
        for (int i = 0; i < dirs_per_cluster; i++) {
            if (directories[i].inode == inode.nodeid) {
                return directories[i].item_name;
            }
        }
    }
    if (parrent.indirect1 != 0) {
        uint32_t links_per_cluster = filesystem_data.super_block.cluster_size / sizeof(int32_t);
        int32_t links[links_per_cluster];
        fs_file.seekp(parrent.indirect1);
        fs_file.read(reinterpret_cast<char *>(&links), sizeof(links));
        for (int i = 0; i < links_per_cluster; i++) {
            if (links[i] != 0) {
                fs_file.seekp(links[i]);
                fs_file.read(reinterpret_cast<char *>(&directories), sizeof(directories));
                for (int i = 0; i < dirs_per_cluster; i++) {
                    if (directories[i].inode == inode.nodeid) {
                        return directories[i].item_name;
                    }
                }
            }
        }
    }
    if (parrent.indirect2 != 0) {
        uint32_t links_per_cluster = filesystem_data.super_block.cluster_size / sizeof(int32_t);
        int32_t links[links_per_cluster];
        fs_file.seekp(parrent.indirect2);
        fs_file.read(reinterpret_cast<char *>(&links), sizeof(links));
        for (int i = 0; i < links_per_cluster; i++) {
            if (links[i] != 0) {
                int32_t sublinks[links_per_cluster];
                fs_file.seekp(links[i]);
                fs_file.read(reinterpret_cast<char *>(&sublinks), sizeof(sublinks));
                for (int j = 0; j < links_per_cluster; j++) {
                    if (sublinks[j] != 0) {
                        fs_file.seekp(sublinks[j]);
                        fs_file.read(reinterpret_cast<char *>(&directories), sizeof(directories));
                        for (int i = 0; i < dirs_per_cluster; i++) {
                            if (directories[i].inode == inode.nodeid) {
                                return directories[i].item_name;
                            }
                        }
                    }
                }
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
    std::fstream fs_file;
    fs_file.open(filesystem_data.fs_file, std::ios::in | std::ios::out | std::ios::binary);

    std::stack<pseudo_inode> s_path;

    pseudo_inode current = inode;
    pseudo_inode root = filesystem_data.root_dir;

    s_path.push(current);
    do {
        current = *getParrentDirectory(filesystem_data, fs_file, current);
        s_path.push(current);
    } while (current.nodeid != root.nodeid);

    if (verbose) {
        std::string path = getPathString(filesystem_data, fs_file, s_path);
        std::cout << path << '\n';
    }

    fs_file.close();
}

/**
 * Vypise aktualni cestu
 * @param filesystem_data filesystem
 * @return cesta
 */
std::string pwd(filesystem &filesystem_data) {
    return pwd(filesystem_data, filesystem_data.current_dir, true);
}