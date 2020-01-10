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
 * Zmeni aktualni cestu do adresare a1
 * @param filesystem_data filesystem
 * @param a1 nazev adresare
 * @param verbose vypisovani zprav
 * @param relocate premisteni se
 * @return reference na inode adresare
 */
pseudo_inode *cd(filesystem &filesystem_data, std::string &a1, bool verbose, bool relocate) {
    // cesta rozdelena na adresare
    std::vector<std::string> segments = splitPath(a1);
    // otevreni souboru fs
    std::fstream fs_file;
    fs_file.open(filesystem_data.fs_file, std::ios::in | std::ios::out | std::ios::binary);

    // pracovni adresar nad nimz jsou provadeny zmeny
    pseudo_inode working_dir = filesystem_data.current_dir;
    // vynucene ukonceni cyklu pri neplatne ceste
    int force_break = 0;
    // prochazeni adresarema
    for (int i = 0; i < segments.size(); i++) {
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
    if (verbose) {
        if (force_break) {
            if (force_break == 2) {
                std::cout << "FILE IS NOT DIRECTORY" << std::endl;
            } else {
                std::cout << "PATH NOT FOUND" << std::endl;
            }
        } else {
            std::cout << "OK" << std::endl;
        }
    }
    if (relocate) {
        filesystem_data.current_dir = working_dir;
    }
    // uzavreni souboru fs
    fs_file.close();

    // reference na pracovni adresar
    pseudo_inode * working_dir_ptr = &working_dir;
    return working_dir_ptr;
}

/**
 * Zmeni aktualni cestu do adresare a1
 * @param filesystem_data filesystem
 * @param a1 nazev adresare
 */
void cd(filesystem &filesystem_data, std::string &a1) {
    cd(filesystem_data, a1, true, true);
}