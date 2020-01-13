//
// Created by pavel on 28.12.19.
//

#include <fstream>
#include <vector>
#include <iostream>
#include "inode.h"
#include "zosfsstruct.h"
#include "directory.h"

/**
 * Vrati pozici inodu v souboru podle poradi inodu
 * @param filesystem_data filesystem
 * @param inode_no poradi inodu
 * @return pozice v souboru fs
 */
int32_t getINodePosition(filesystem &filesystem_data, int32_t inode_no) {
    return filesystem_data.super_block.inode_start_address +
           (inode_no - 1) * sizeof(pseudo_inode);
}

/**
 * Vrati volny inode
 * @return inode
 */
pseudo_inode *getFreeINode(filesystem &filesystem_data) {
    std::fstream input_file;
    input_file.open(filesystem_data.fs_file, std::ios::in);

    input_file.seekp(filesystem_data.super_block.inode_start_address);

    // Pocet inodu jako misto mezi zacatkem datove casti a zacatkem prvniho inodu
    int32_t inodes_count =
            (filesystem_data.super_block.data_start_address - filesystem_data.super_block.inode_start_address) /
            sizeof(pseudo_inode);

    // nactu pole inodu
    pseudo_inode inodes[inodes_count];
    input_file.read(reinterpret_cast<char *>(&inodes), sizeof(inodes));

    // Hledam inode
    pseudo_inode inode;
    pseudo_inode *inode_ptr = nullptr;
    for (int i = 0; i < inodes_count; i++) {
        if (inodes[i].nodeid == 0) {     // volny inode ma ID == 0
            inode = inodes[i];
            inode.nodeid = (i + 1);       // pridam inodu poradove cislo jako ID
            inode_ptr = &inode;
            break;
        }
    }

    input_file.close();
    return inode_ptr;
}

/**
 * Vrati referenci na inode souboru
 * @param filesystem_data filesystem
 * @param working_dir pracovni adresar
 * @param s1 nazev souboru
 * @param verbose vypisovani zprav
 * reference na inode souboru
 */
pseudo_inode *getFileINode(filesystem &filesystem_data, pseudo_inode &working_dir, std::string &s1, bool verbose) {
    std::fstream input_file;
    input_file.open(filesystem_data.fs_file, std::ios::in | std::ios::out | std::ios::binary);

    pseudo_inode inode;
    pseudo_inode *inode_ptr = nullptr;

    // zjistim, zdali existuje adresar stejneho nazvu
    std::vector<directory_item> directories = getDirectories(filesystem_data, working_dir);
    for (auto &directory : directories) {
        directory.item_name;
        if (strcmp(s1.c_str(), directory.item_name) == 0) {
            input_file.seekp(getINodePosition(filesystem_data, directory.inode));
            input_file.read(reinterpret_cast<char *>(&inode), sizeof(pseudo_inode));
            if (inode.type == 1) {
                if (verbose) {
                    std::cout << "FILE IS DIRECTORY" << std::endl;
                }
            } else {
                inode_ptr = &inode;
            }
            break;
        }
    }

    input_file.close();
    return inode_ptr;
}

/**
 * Vrati referenci na inode adresare nebo souboru
 * @param filesystem_data filesystem
 * @param location lokace / nazev souboru
 * @param verbose vypisovani zprav
 */
pseudo_inode *iNodeByLocation(filesystem &filesystem_data, std::string &location, bool verbose) {
    // cesta rozdelena na adresare
    std::vector<std::string> segments = splitPath(location);
    // otevreni souboru fs
    std::fstream fs_file;
    fs_file.open(filesystem_data.fs_file, std::ios::in | std::ios::out | std::ios::binary);

    // pracovni adresar nad nimz jsou provadeny zmeny
    pseudo_inode working_dir = filesystem_data.current_dir;
    pseudo_inode *working_dir_ptr = &working_dir;
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
                working_dir = dir_inode;
                force_break = 0;        // OK
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
            std::cout << "PATH NOT FOUND" << std::endl;
        } else {
            std::cout << "OK" << std::endl;
        }
    }
    // uzavreni souboru fs
    fs_file.close();

    // vrati referenci na pracovni adresar / soubor
    if (force_break) {
        working_dir_ptr = nullptr;
    }
    return working_dir_ptr;
}

/**
 * Uvolni inode
 * @param filesystem_data filesystem
 * @param fs_file otevreny soubor filesystemu
 * @param inode adresar
 */
void removeINode(filesystem &filesystem_data, std::fstream &fs_file, pseudo_inode &inode) {
    int32_t inode_position = getINodePosition(filesystem_data, inode.nodeid);
    inode.references = inode.references - 1;
    if (inode.references < 1) {
        inode.direct1 = 0;
        inode.direct2 = 0;
        inode.direct3 = 0;
        inode.direct4 = 0;
        inode.direct5 = 0;
        inode.indirect1 = 0;
        inode.indirect2 = 0;
        inode.file_size = 0;
        inode.nodeid = 0;
    }
    fs_file.seekp(inode_position);
    fs_file.write(reinterpret_cast<const char *>(&inode), sizeof(pseudo_inode));
}