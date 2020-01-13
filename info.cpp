//
// Created by pavel on 06.01.20.
//

#include <fstream>
#include <vector>
#include <iostream>
#include "info.h"
#include "zosfsstruct.h"
#include "directory.h"
#include "inode.h"
#include "datablock.h"
#include "cd.h"
#include "pwd.h"

/**
 * Vypise adresy databloku
 * @param filesystem_data filesystem
 * @param fs_file otevreny soubor filesystemu
 * @param inode soubor
 */
void printDatablocksAddresses(filesystem &filesystem_data, std::fstream &fs_file, pseudo_inode &inode) {
    // platne adresy na databloky
    std::vector<int32_t> addresses = usedDatablockByINode(filesystem_data, fs_file, inode, true);
    std::cout << addresses.size() << " BLOCKS | ";
    // prochazeni adres databloku
    for (auto &address : addresses) {
        std::cout << address << " ";
    }
}

/**
 * Vypise informace o souboru/adresari s1/a1
 * @param filesystem_data filesystem
 * @param name nazev
 */
void info(filesystem &filesystem_data, std::string &name) {

    // cesta rozdelena na adresare
    std::vector<std::string> segments = splitPath(name);

    // pracovni adresar (nadrazeny adresar)
    pseudo_inode *working_dir_ptr = cd(filesystem_data, name, false, false);
    pseudo_inode working_dir = filesystem_data.current_dir;
    if (working_dir_ptr != nullptr) {
        working_dir = *working_dir_ptr;
    }

    // dotazovany adresar / soubor
    pseudo_inode *quered_inode_ptr = iNodeByLocation(filesystem_data, name, false);
    pseudo_inode quered_inode = filesystem_data.current_dir;
    if (quered_inode_ptr != nullptr) {
        quered_inode = *quered_inode_ptr;
    }

    if (quered_inode_ptr != nullptr && working_dir_ptr != nullptr) {
        std::fstream fs_file;
        fs_file.open(filesystem_data.fs_file, std::ios::in | std::ios::out | std::ios::binary);

        // posunuti o adresar zpet, zkoumame-li adresar
        if (quered_inode.type == 1) {
            pseudo_inode *working_dir_ptr = getParrentDirectory(filesystem_data, fs_file, working_dir);
            if (working_dir_ptr != nullptr) {
                working_dir = *working_dir_ptr;
            }
        }

        // zjistim, zdali existuje adresar stejneho nazvu
        std::string dir_name = "";
        std::vector<directory_item> directories = getDirectories(filesystem_data, working_dir);
        for (auto &directory : directories) {
            if (strcmp(segments.back().c_str(), directory.item_name) == 0) {
                dir_name = directory.item_name;
                break;
            }
        }

        std::cout << "NAME\t – SIZE\t – INODE\t – LINKS" << std::endl;
        std::cout << dir_name << "\t – " << quered_inode.file_size << "\t – " << quered_inode.nodeid << "\t – ";
        printDatablocksAddresses(filesystem_data, fs_file, quered_inode);
        std::cout << std::endl;

        fs_file.close();
    } else {
        std::cout << "FILE NOT FOUND" << std::endl;
    }
}