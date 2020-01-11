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

pseudo_inode *moveToParrent(filesystem &filesystem_data, std::string &s1) {
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
    pseudo_inode *a1_path_ptr = moveToParrent(filesystem_data, s1);
    pseudo_inode a1_path;
    directory_item s1_dir;
    if(a1_path_ptr != nullptr) {
        a1_path = *a1_path_ptr;        // reference na nadrazeny adresar
        directory_item * s1_dir_ptr = getDirectoryItem(filesystem_data,a1_path, s1_segments.back());
        if(s1_dir_ptr != nullptr) {
            s1_dir = *s1_dir_ptr;
            std::cout << "File name: " << s1_dir.item_name << std::endl;
        } else {
            return;
        }
    } else {
        return;
    }

    std::vector<std::string> s2_segments = splitPath(s2);
    pseudo_inode *a2_path_ptr = moveToParrent(filesystem_data, s2);
    if(a2_path_ptr != nullptr) {
        pseudo_inode a2_path = *a2_path_ptr;        // reference na nadrazeny adresar
        directory_item * s2_dir_ptr = getDirectoryItem(filesystem_data,a2_path, s2_segments.back());
        if(s2_dir_ptr == nullptr) {
            directory_item s2_dir = createDirectoryItem(s1_dir.inode, s2_segments.back());    // nazev souboru
            std::cout << "File name: " << s2_dir.item_name << " Inode" << s2_dir.inode << std::endl;
            int32_t address = addDirectoryItemEntry(filesystem_data, a2_path, s2_dir);
            if(address) {
                removeDirectoryItemEntry(filesystem_data, a1_path, s1_dir);
                std::cout << "OK" << std::endl;
            }

        } else {
            directory_item s2_dir = *s2_dir_ptr;
            std::cout << "File exist " << s2_dir.item_name << "!" << std::endl;
        }
    }

}

/**
 * Zkopireje zdrojovy datablok do ciloveho databloku
 * @param filesystem_data informace filesystemu
 * @param fs_file otevreny soubor na pseudoNTFS
 * @param source_address pocatecni pozice zdrojoveho datablocku
 * @param target_address pocatecni pozice ciloveho datablocku
 */
void copyDataBlock(filesystem &filesystem_data, std::fstream &fs_file, int32_t source_address, int32_t target_address) {
    int32_t block_size = filesystem_data.super_block.cluster_size;
    // priprava bufferu
    char buffer[block_size];
    memset(buffer, EOF, block_size);
    // presun na pozici zdroje
    fs_file.seekp(source_address);
    fs_file.read(buffer, block_size);
    // presun na pozici cile
    fs_file.seekp(target_address);
    fs_file.write(buffer, block_size);

    //std::cout << "OLD ADDRESS: " << source_address << " NEW ADDRESS: " << target_address << std::endl;
}

/**
 * Zkopiruje soubor s1 do umisteni s2
 * @param filesystem_data filesystem
 * @param s1 nazev souboru
 * @param s2 nazev souboru
 */
void cp(filesystem &filesystem_data, std::string &s1, std::string &s2) {

    // cesta rozdelena na adresare
    std::vector<std::string> from_segments = splitPath(s1);
    std::vector<std::string> to_segments = splitPath(s2);

    // pracovni adresare
    pseudo_inode *from_dir_ptr = cd(filesystem_data, s1, false, false);
    pseudo_inode from_dir = filesystem_data.current_dir;
    if(from_dir_ptr != nullptr) {
        from_dir = *from_dir_ptr;
    } else {
        std::cout << "SOURCE DIRECTORY DOES NOT EXISTS!" << std::endl;
        return;
    }

    pseudo_inode *to_dir_ptr = cd(filesystem_data, s2, false, false);
    pseudo_inode to_dir = filesystem_data.current_dir;
    if(to_dir_ptr != nullptr) {
        to_dir = *to_dir_ptr;
    } else {
        std::cout << "DESTINATION DIRECTORY DOES NOT EXISTS!" << std::endl;
        return;
    }

    // najdu odpovidajici inode zdroje
    pseudo_inode source_inode;
    pseudo_inode *source_inode_ptr = getFileINode(filesystem_data, from_dir, from_segments.back());
    if (source_inode_ptr != nullptr) {
        source_inode = *source_inode_ptr;
    } else {
        std::cout << "SOURCE FILE DOES NOT EXISTS!" << std::endl;
        return;
    }

    // platne adresy na databloky
    std::fstream fs_file;
    fs_file.open(filesystem_data.fs_file, std::ios::in | std::ios::out | std::ios::binary);
    std::vector<int32_t> source_addresses = usedDatablockByINode(filesystem_data, fs_file, source_inode, true);
    int32_t blocks_used = source_addresses.size();

    // dostupnych volnych databloku
    int32_t blocks_available = availableDatablocks(filesystem_data);

    if(blocks_available < blocks_used) {
        std::cout << "NOT ENOUGH SPACE!" << std::endl;
        return;
    }

    // najdu volny inode
    pseudo_inode target_inode;
    pseudo_inode *target_inode_ptr;
    target_inode_ptr = getFreeINode(filesystem_data);
    if (target_inode_ptr != nullptr) {
        target_inode = *target_inode_ptr;
        target_inode.isDirectory = source_inode.isDirectory;
        target_inode.file_size = source_inode.file_size;
        std::cout << "New INODE ID: " << target_inode.nodeid << std::endl;
        fs_file.seekp(getINodePosition(filesystem_data, target_inode.nodeid));
        fs_file.write(reinterpret_cast<const char *>(&target_inode), sizeof(pseudo_inode));
        directory_item target_dir = createDirectoryItem(target_inode.nodeid, to_segments.back());
        int32_t address = addDirectoryItemEntry(filesystem_data, to_dir, target_dir);
    } else {
        std::cout << "NO FREE INODE LEFT!" << std::endl;
        return;
    }

    // platne adresy na databloky
    source_addresses = usedDatablockByINode(filesystem_data, fs_file, source_inode, false);
    std::cout << source_addresses.size() << " BLOCKS | ";
    // prochazeni adres databloku
    for (auto &address : source_addresses) {
        std::cout << address << " ";
        int32_t obtained_address = addDatablockToINode(filesystem_data, fs_file, target_inode);
        copyDataBlock(filesystem_data, fs_file, address, obtained_address);
    }

    fs_file.close();
}


