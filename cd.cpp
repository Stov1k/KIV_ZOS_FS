//
// Created by pavel on 07.01.20.
//

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include "zosfsstruct.h"
#include "directory.h"
#include "inode.h"
#include "cd.h"

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
 * Zmeni aktualni cestu do adresare a1
 * @param filesystem_data filesystem
 * @param a1 nazev adresare
 */
void cd(filesystem &filesystem_data, std::string &a1) {
    // cesta rozdelena na adresare
    std::vector<std::string> segments = splitPath(a1);
    // otevreni souboru fs
    std::fstream fs_file;
    fs_file.open(filesystem_data.fs_file, std::ios::in | std::ios::out | std::ios::binary);
    // vynucene ukonceni cyklu pri neplatne ceste
    int force_break = 0;
    // prochazeni adresarema
    for (int i = 0; i < segments.size(); i++) {
        if (i == 0 && segments[i].length() == 0) {   // zadana absolutni cesta
            filesystem_data.current_dir = filesystem_data.root_dir;
            continue;
        } else if (i != 0 && segments[i].length() == 0) {    // ignorovani nasobnych lomitek
            continue;
        }
        // zjistim, zdali existuje adresar stejneho nazvu
        std::vector<directory_item> directories = getDirectories(filesystem_data);
        for (auto &directory : directories) {
            force_break = 1;    // PATH NOT FOUND (v pripade nalezeni se prepise na 0 nebo 2)
            if (strcmp(segments[i].c_str(), directory.item_name) == 0) {
                fs_file.seekp(getINodePosition(filesystem_data, directory.inode));
                pseudo_inode dir_inode;
                fs_file.read(reinterpret_cast<char *>(&dir_inode), sizeof(pseudo_inode));
                if (dir_inode.isDirectory) {
                    filesystem_data.current_dir = dir_inode;
                    force_break = 0;        // OK
                } else {
                    force_break = 2;        // FILE IS NOT DIRECTORY
                }
                break;
            }
        }
        if (force_break) {
            break;
        }
    }
    // vypsani zpravy
    if (force_break) {
        if (force_break == 2) {
            std::cout << "FILE IS NOT DIRECTORY" << std::endl;
        } else {
            std::cout << "PATH NOT FOUND" << std::endl;
        }
    } else {
        std::cout << "OK" << std::endl;
    }
    // uzavreni souboru fs
    fs_file.close();
}