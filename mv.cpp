//
// Created by pavel on 08.01.20.
//

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include "mv.h"
#include "zosfsstruct.h"
#include "directory.h"
#include "inode.h"

pseudo_inode *moveTo(filesystem &filesystem_data, std::string &s1) {
    // cesta rozdelena na adresare
    std::vector<std::string> segments = splitPath(s1);
    // otevreni souboru fs
    std::fstream fs_file;
    fs_file.open(filesystem_data.fs_file, std::ios::in | std::ios::out | std::ios::binary);

    // pracovni adresar nad nimz jsou provadeny zmeny
    pseudo_inode *working_dir_ptr;
    pseudo_inode working_dir = filesystem_data.current_dir;
    // vynucene ukonceni cyklu pri neplatne ceste
    int force_break = 0;
    // prochazeni adresarema
    for (int i = 0; i < segments.size() - 1; i++) {
        if (i == 0 && segments[i].length() == 0) {   // zadana absolutni cesta
            working_dir = filesystem_data.root_dir;
            continue;
        } else if (i != 0 && segments[i].length() == 0) {    // ignorovani nasobnych lomitek
            continue;
        }
        // zjistim, zdali existuje adresar stejneho nazvu
        std::vector<directory_item> directories = getDirectories(filesystem_data, working_dir);
        for (auto &directory : directories) {
            force_break = 1;    // PATH NOT FOUND (v pripade nalezeni se prepise na 0 nebo 2)
            if (strcmp(segments[i].c_str(), directory.item_name) == 0) {
                fs_file.seekp(getINodePosition(filesystem_data, directory.inode));
                pseudo_inode dir_inode;
                fs_file.read(reinterpret_cast<char *>(&dir_inode), sizeof(pseudo_inode));
                if (dir_inode.isDirectory) {
                    working_dir = dir_inode;
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
            working_dir_ptr = nullptr;
        }
    } else {
        std::cout << "OK" << std::endl;
        working_dir_ptr = &working_dir;
    }

    // uzavreni souboru fs
    fs_file.close();

    return working_dir_ptr;
}

/**
 * Presune soubor s1 do umisteni s2
 * @param filesystem_data filesystem
 * @param s1 nazev souboru
 * @param s2 nazev souboru
 */
void mv(filesystem &filesystem_data, std::string &s1, std::string &s2) {

    std::vector<std::string> s1_segments = splitPath(s1);
    std::vector<std::string> s2_segments = splitPath(s2);

    pseudo_inode *old_path_ptr = moveTo(filesystem_data, s1);
    pseudo_inode *new_path_ptr = moveTo(filesystem_data, s2);

    // otevreni souboru fs
    std::fstream fs_file;
    fs_file.open(filesystem_data.fs_file, std::ios::in | std::ios::out | std::ios::binary);

    if (old_path_ptr != nullptr && new_path_ptr != nullptr) {
        pseudo_inode old_path = *old_path_ptr;
        std::cout << "Old name: " << s1_segments.back() << " New name: " << s2_segments.back() << std::endl;
        directory_item old_dir = getDirectory(0, s1_segments.back());
        if (isDirectoryExists(filesystem_data, old_path, old_dir)) {
            pseudo_inode *old_inode_ptr = getFileINode(filesystem_data, old_path, s1_segments.back());
            if(old_inode_ptr != nullptr) {
                pseudo_inode old_inode = *old_inode_ptr;
                std::cout << old_inode.nodeid << std::endl;
            }
        } else {
            std::cout << old_dir.item_name << std::endl;
        }
    }

    // uzavreni souboru fs
    fs_file.close();
}