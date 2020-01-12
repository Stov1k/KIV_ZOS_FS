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
#include "cd.h"
#include "datablock.h"

/**
 * Presune soubor s1 do umisteni s2
 * @param filesystem_data filesystem
 * @param s1 nazev souboru
 * @param s2 nazev souboru
 */
void mv(filesystem &filesystem_data, std::string &s1, std::string &s2) {

    std::vector<std::string> s1_segments = splitPath(s1);
    pseudo_inode *a1_path_ptr = cd(filesystem_data, s1, false, false);
    pseudo_inode a1_path;
    directory_item s1_dir;
    if (a1_path_ptr != nullptr) {
        a1_path = *a1_path_ptr;        // reference na nadrazeny adresar
        directory_item *s1_dir_ptr = getDirectoryItem(filesystem_data, a1_path, s1_segments.back());
        if (s1_dir_ptr != nullptr) {
            s1_dir = *s1_dir_ptr;
        } else {
            std::cout << "FILE NOT FOUND" << std::endl;   // SOURCE FILE DOES NOT EXISTS
            return;
        }
    } else {
        std::cout << "FILE NOT FOUND" << std::endl;  // SOURCE DIRECTORY DOES NOT EXISTS
        return;
    }

    std::vector<std::string> s2_segments = splitPath(s2);
    pseudo_inode *a2_path_ptr = cd(filesystem_data, s2, false, false);
    if (a2_path_ptr != nullptr) {
        pseudo_inode a2_path = *a2_path_ptr;        // reference na nadrazeny adresar
        directory_item *s2_dir_ptr = getDirectoryItem(filesystem_data, a2_path, s2_segments.back());
        if (s2_dir_ptr == nullptr) {
            directory_item s2_dir = createDirectoryItem(s1_dir.inode, s2_segments.back());    // nazev souboru
            int32_t address = addDirectoryItemEntry(filesystem_data, a2_path, s2_dir);
            if (address) {
                removeDirectoryItemEntry(filesystem_data, a1_path, s1_dir);
                std::cout << "OK" << std::endl;
            }

        } else {
            directory_item s2_dir = *s2_dir_ptr;
            std::cout << "FILE ALREADY EXISTS" << std::endl;
        }
    } else {
        std::cout << "PATH NOT FOUND" << std::endl;  // DESTINATION DIRECTORY DOES NOT EXISTS
        return;
    }

}


