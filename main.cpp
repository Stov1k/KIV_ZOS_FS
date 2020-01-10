#include <iostream>
#include <vector>
#include <iterator>
#include <sstream>
#include <fstream>
#include <cmath>
#include <cstring>
#include <bitset>
#include "zosfsstruct.h"
#include "incp.h"
#include "inode.h"
#include "directory.h"
#include "datablock.h"
#include "cat.h"
#include "outcp.h"
#include "rm.h"
#include "info.h"
#include "pwd.h"
#include "cd.h"
#include "mkdir.h"
#include "mv.h"
#include "ls.h"
#include <sys/stat.h>
#include <cstdio>

filesystem filesystem_data;

/**
 * Vypise bitmapu
 */
void print_bitmap() {
    std::fstream input_file;
    input_file.open(filesystem_data.fs_file, std::ios::in);
    if (!input_file.is_open()) {
        std::cout << "Nelze otevrit." << std::endl;
    } else {
        // nacteni
        input_file.read(reinterpret_cast<char *>(&filesystem_data.super_block), sizeof(superblock));
        // skok na bitmapu
        input_file.seekp(filesystem_data.super_block.bitmap_start_address);

        int bitmap_size_bytes = (filesystem_data.super_block.inode_start_address -
                                 filesystem_data.super_block.bitmap_start_address);

        for (int i = 0; i < bitmap_size_bytes; i++) {
            char b;
            input_file.read(&b, 1);
            std::bitset<8> x(b);
            unsigned long ul = x.to_ulong();
            unsigned int c = static_cast<unsigned int>(ul);
            std::cout << x << " pos:" << input_file.tellg() << " Long:" << ul << " Int:" << c << std::endl;
        }
    }
    input_file.close();
}

void openFS() {
    std::fstream input_file;
    input_file.open(filesystem_data.fs_file, std::ios::in | std::ios::out | std::ios::binary);

    // nacteni
    input_file.read(reinterpret_cast<char *>(&filesystem_data.super_block), sizeof(superblock));
    if (!input_file.is_open()) {
        std::cout << "FS neni otevren. " << std::endl;
    }
    std::cout << "Podpis: " << filesystem_data.super_block.signature << std::endl;

    //nastaveni korenoveho adresare
    input_file.seekp(filesystem_data.super_block.inode_start_address);
    pseudo_inode root_inode;
    input_file.read(reinterpret_cast<char *>(&root_inode), sizeof(pseudo_inode));
    filesystem_data.current_dir = root_inode;
    filesystem_data.root_dir = root_inode;

    input_file.close();
}

/**
 * Naformatovani souboru
 * @param size velikost disku
 */
void formatFS(int size) {
    int remaining_space = size;

    std::fstream input_file;

    // Vytvoreni / format souboru
    input_file.open(filesystem_data.fs_file, std::ios::binary | std::ios::trunc | std::ios::in | std::ios::out);
    input_file.close();

    // otevreni souboru pro zapis
    input_file.open(filesystem_data.fs_file, std::ios::binary | std::ios::trunc | std::ios::in | std::ios::out);

    // prazdny 1B pouzivany pri zapisu
    uint8_t byte{0};

    // super block
    filesystem_data.super_block = superblock{};
    std::string signature = "zelenka";
    std::string description = "muj fs";
    std::strncpy(filesystem_data.super_block.signature, signature.c_str(),
                 sizeof(filesystem_data.super_block.signature));
    filesystem_data.super_block.signature[sizeof(filesystem_data.super_block.signature) - 1] = '\0';
    std::strncpy(filesystem_data.super_block.volume_descriptor, description.c_str(),
                 sizeof(filesystem_data.super_block.volume_descriptor));
    filesystem_data.super_block.volume_descriptor[sizeof(filesystem_data.super_block.volume_descriptor) - 1] = '\0';

    int cluster_size = 1024;
    remaining_space = remaining_space - sizeof(superblock);
    int inodes = remaining_space / cluster_size;
    remaining_space = remaining_space - sizeof(pseudo_inode);
    int bitmap_size = remaining_space / cluster_size;
    int correction = bitmap_size %
                     8;                     // velikost FS bude vzdy delitelna 8 (bitmapa pak vyuzije vzdy cely byte)
    bitmap_size = bitmap_size - correction;
    int bitmap_size_bytes = bitmap_size / 8;

    int clusters = bitmap_size;

    filesystem_data.super_block.disk_size = sizeof(superblock) + bitmap_size + inodes + cluster_size * clusters;
    filesystem_data.super_block.cluster_size = cluster_size;
    filesystem_data.super_block.cluster_count = clusters;
    filesystem_data.super_block.bitmap_start_address = sizeof(filesystem_data.super_block);
    filesystem_data.super_block.inode_start_address =
            sizeof(filesystem_data.super_block) + sizeof(byte) * bitmap_size_bytes;
    filesystem_data.super_block.data_start_address =
            sizeof(filesystem_data.super_block) + sizeof(byte) * bitmap_size_bytes + sizeof(pseudo_inode) * inodes;
    input_file.write(reinterpret_cast<const char *>(&filesystem_data.super_block), sizeof(superblock));


    // zapis inodu
    pseudo_inode inode;
    for (int i = 0; i < inodes; i++) {
        inode = pseudo_inode{};
        inode.nodeid = ID_ITEM_FREE;
        input_file.write(reinterpret_cast<const char *>(&inode), sizeof(pseudo_inode));
    }

    // zapis bitmapy
    for (int bmp = 0; bmp < bitmap_size_bytes; bmp++) {
        std::cout << (int) byte << " pos:" << input_file.tellg() << std::endl;
        input_file.write(reinterpret_cast<const char *>(&byte), sizeof(byte));
    }

    // zapis vsech clusteru (data bloku)
    for (int i = 0; i < clusters; i++) {
        // zapis jednoho konkretniho data bloku
        for (int j = 0; j < cluster_size; j++) {
            input_file.write(reinterpret_cast<const char *>(&byte), sizeof(byte));
        }
    }
    input_file.flush();

    // vytvoreni korenoveho adresare
    directory_item dir;
    inode.nodeid = 1;
    inode.isDirectory = true;
    inode.references++;
    inode.direct1 = filesystem_data.super_block.data_start_address;
    inode.direct2 = 0;
    inode.direct3 = 0;
    inode.direct4 = 0;
    inode.direct5 = 0;
    inode.indirect1 = 0;
    inode.indirect2 = 0;
    inode.file_size = filesystem_data.super_block.cluster_size;

    // aktualni adresar
    dir = createDirectoryItem(inode.nodeid, ".");

    int32_t dirs_per_cluster = cluster_size / sizeof(directory_item);

    // predchozi adresar
    directory_item dir_parrent;
    dir_parrent = createDirectoryItem(inode.nodeid, "..");;

    // nastaveni referenci na adresare
    directory_item directories[dirs_per_cluster];
    directories[0] = dir;                           // .
    directories[1] = dir_parrent;                   // ..
    for (int i = 2; i < dirs_per_cluster; i++) {     // dalsi neobsazene
        directories[i] = directory_item{};
        directories[i].inode = 0;   // tj. nic
    }
    filesystem_data.current_dir = inode;

    // vytvoreni korenoveho adresare: uprava bitmapy
    input_file.seekp(filesystem_data.super_block.bitmap_start_address); // skoci na bitmapu
    uint8_t byte1{1};       // TODO: neni idealni, chci menit jen 1 bit, ne 1 byte
    std::cout << (int) byte1 << " pos:" << input_file.tellg() << std::endl;
    input_file.write(reinterpret_cast<const char *>(&byte1), sizeof(byte1));    // zapise upravenou bitmapu

    // vytvoreni korenoveho adresare: zapis dat
    input_file.seekp(filesystem_data.super_block.data_start_address); // skoci na data
    input_file.write(reinterpret_cast<const char *>(&directories), sizeof(directories));

    // vytvoreni korenoveho adresare: zapis inode
    input_file.seekp(filesystem_data.super_block.inode_start_address); // skoci na inody
    input_file.write(reinterpret_cast<const char *>(&inode), sizeof(inode));

    input_file.close();

    std::cout << "Popis: " << bitmap_size_bytes << std::endl;

}

/**
 * Hlavni
 * @param argc
 * @param argv
 * @return konec
 */
int main(int argc, char **argv) {

    filesystem_data = filesystem{};

    // Nacteni nazvu souboru
    if (argc >= 2) {
        filesystem_data.fs_file.append(argv[1]);
    } else {
        char myword[] = "my.fs";
        filesystem_data.fs_file.append(myword);
    }
    std::cout << "File: " << filesystem_data.fs_file << std::endl;

    // Otevreni souboru pro cteni
    std::fstream input_file;
    input_file.open(filesystem_data.fs_file, std::ios::in);
    if (!input_file.is_open()) {
        std::cout << "Nelze otevrit." << std::endl;
    } else {
        std::cout << "Uspesne otevreno." << std::endl;
    }
    input_file.close();

    // Nacteni fs
    openFS();

    std::vector<directory_item> directories = getDirectories(filesystem_data, filesystem_data.current_dir);
    printDirectories(directories);

    // Smycka prikazu
    while (true) {
        std::cout << "> ";
        std::string input_string;
        std::getline(std::cin, input_string);  // nacte cely radek

        std::istringstream iss(input_string);
        std::vector<std::string> cmd(std::istream_iterator<std::string>{iss}, std::istream_iterator<std::string>());

        if (cmd.size() > 0 && cmd[0] == "format") {
            std::cout << cmd.size() << std::endl;
            if (cmd.size() == 2) {
                std::cout << "FORMAT..." << std::endl;
                int fs_size = 0;
                std::string str_size("");
                for (int i = 0; i < cmd[1].length(); i++) {
                    if (isdigit(cmd[1].at(i))) {
                        str_size.push_back(cmd[1].at(i));
                    } else {
                        fs_size = std::stoi(str_size);
                        if (cmd[1].at(i) == 'M') {
                            fs_size = fs_size * 1048576;
                            std::cout << "MB" << std::endl;
                        } else if (cmd[1].at(i) == 'K') {
                            fs_size = fs_size * 1024;
                            std::cout << "KB" << std::endl;
                        }
                        break;
                    }
                }
                std::cout << fs_size << std::endl;
                formatFS(fs_size);
                openFS();
            }
        } else if (input_string == "open") {
            std::cout << "OPEN..." << std::endl;
            openFS();
        } else if (input_string == "bitmap") {
            std::cout << "Bitmapa... " << std::endl;
            print_bitmap();
        } else if (cmd.size() > 0 && cmd[0] == "ls") {
            int bitmap_size_bytes = (filesystem_data.super_block.inode_start_address -
                                     filesystem_data.super_block.bitmap_start_address);
            std::cout << "Obsah " << bitmap_size_bytes << std::endl;
            ls(filesystem_data, filesystem_data.current_dir);
        } else if (cmd.size() > 0 && cmd[0] == "pwd") {
            pwd(filesystem_data);
        } else if (cmd.size() > 0 && cmd[0] == "mkdir") {
            if (cmd.size() == 1) {
                std::cout << "mkdir: missing operand" << std::endl;
            } else {
                mkdir(filesystem_data, cmd[1]);
            }
        } else if (cmd.size() > 0 && cmd[0] == "rmdir") {
            if (cmd.size() == 1) {
                std::cout << "rmdir: missing operand" << std::endl;
            } else {
                rmdir(filesystem_data, cmd[1]);
            }
        } else if (cmd.size() > 0 && cmd[0] == "rm") {
            if (cmd.size() == 1) {
                std::cout << "rm: missing operand" << std::endl;
            } else {
                rm(filesystem_data, cmd[1]);
            }
        } else if (cmd.size() > 0 && cmd[0] == "info") {
            if (cmd.size() == 1) {
                std::cout << "info: missing operand" << std::endl;
            } else {
                info(filesystem_data, cmd[1]);
            }
        } else if (cmd.size() > 0 && cmd[0] == "incp") {
            if (cmd.size() < 3) {
                std::cout << "PATH NOT FOUND" << std::endl;
            } else {
                inputCopy(filesystem_data, cmd[1], cmd[1]); // TODO: upravit pak parametry :)
            }
        } else if (cmd.size() > 0 && cmd[0] == "outcp") {
            if (cmd.size() < 3) {
                std::cout << "PATH NOT FOUND" << std::endl;
            } else {
                outcp(filesystem_data, cmd[1], cmd[2]);
            }
        } else if (cmd.size() > 0 && cmd[0] == "mv") {
            if (cmd.size() < 3) {
                std::cout << "PATH NOT FOUND" << std::endl;
            } else {
                mv(filesystem_data, cmd[1], cmd[2]);
            }
        } else if (cmd.size() > 0 && cmd[0] == "cp") {
            if (cmd.size() < 3) {
                std::cout << "PATH NOT FOUND" << std::endl;
            } else {
                cp(filesystem_data, cmd[1], cmd[2]);
            }
        } else if (cmd.size() > 0 && cmd[0] == "cd") {
            if (cmd.size() == 1) {
                filesystem_data.current_dir = filesystem_data.root_dir;
                std::cout << "OK" << std::endl;
            } else {
                cd(filesystem_data, cmd[1]);
            }
        } else if (cmd.size() > 1 && cmd[0] == "cat") {
            cat(filesystem_data, cmd[1]);
        } else if (input_string == "q") {
            std::cout << "Konec" << std::endl;
            return 0;
        };
    }

    return 0;
}