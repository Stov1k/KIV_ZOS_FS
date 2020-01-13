#include <iostream>
#include <vector>
#include <iterator>
#include <sstream>
#include <fstream>
#include <cmath>
#include <cstring>
#include <bitset>
#include <sys/stat.h>
#include <cstdio>
#include <experimental/filesystem>
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
#include "cp.h"
#include "ls.h"
#include "format.h"
#include "slink.h"

int32_t command(std::string cmd_string);

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
    std::cout << "Podpis FS: " << filesystem_data.super_block.signature << std::endl;

    //nastaveni korenoveho adresare
    input_file.seekp(filesystem_data.super_block.inode_start_address);
    pseudo_inode root_inode;
    input_file.read(reinterpret_cast<char *>(&root_inode), sizeof(pseudo_inode));
    filesystem_data.current_dir = root_inode;
    filesystem_data.root_dir = root_inode;

    input_file.close();
}

void load(std::string s1) {

    // existuje cesta ke vstupnimu souboru na pevnem disku?
    if (!std::experimental::filesystem::exists(s1)) {
        std::cout << "FILE NOT FOUND" << std::endl;
        return;
    }
    // nacteni prikazu ze souboru
    std::ifstream file(s1);
    std::string cmd_string = "";
    while (std::getline(file, cmd_string)) {
        if (cmd_string.size() > 0) {
            std::cout << "> " << cmd_string << std::endl;
            command(cmd_string);
        }
    }

    std::cout << "OK" << std::endl;
}

/**
 * Zpracovani prikazu
 * @param cmd_string prikaz
 * @return ukonceni aplikace?
 */
int32_t command(std::string cmd_string) {
    std::istringstream iss(cmd_string);
    std::vector<std::string> cmd(std::istream_iterator<std::string>{iss}, std::istream_iterator<std::string>());

    if (cmd.size() > 0 && cmd[0] == "format") {
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
                    } else if (cmd[1].at(i) == 'K') {
                        fs_size = fs_size * 1024;
                    }
                    break;
                }
            }
            format(filesystem_data, fs_size);
            openFS();
        } else {
            std::cout << "mkdir: wrong input" << std::endl;
        }
    } else if (cmd[0] == "open") {
        std::cout << "OPEN..." << std::endl;
        openFS();
    } else if (cmd[0] == "bitmap") {
        std::cout << "Bitmapa... " << std::endl;
        print_bitmap();
    } else if (cmd.size() > 0 && cmd[0] == "ls") {
        if (cmd.size() == 1) {
            ls(filesystem_data);
        } else {
            ls(filesystem_data, cmd[1]);
        }
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
            std::cout << "incp: missing operand" << std::endl;
        } else {
            incp(filesystem_data, cmd[1], cmd[2]);
        }
    } else if (cmd.size() > 0 && cmd[0] == "outcp") {
        if (cmd.size() < 3) {
            std::cout << "outcp: missing operand" << std::endl;
        } else {
            outcp(filesystem_data, cmd[1], cmd[2]);
        }
    } else if (cmd.size() > 0 && cmd[0] == "slink") {
        if (cmd.size() < 3) {
            std::cout << "slink: missing operand" << std::endl;
        } else {
            slink(filesystem_data, cmd[1], cmd[2]);
        }
    } else if (cmd.size() > 0 && cmd[0] == "mv") {
        if (cmd.size() < 3) {
            std::cout << "mv: missing operand" << std::endl;
        } else {
            mv(filesystem_data, cmd[1], cmd[2]);
        }
    } else if (cmd.size() > 0 && cmd[0] == "cp") {
        if (cmd.size() < 3) {
            std::cout << "cp: missing operand" << std::endl;
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
    } else if (cmd.size() > 0 && cmd[0] == "cat") {
        if (cmd.size() == 1) {
            std::cout << "cat: missing operand" << std::endl;
        } else {
            cat(filesystem_data, cmd[1]);
        }
    } else if (cmd.size() > 0 && cmd[0] == "load") {
        if (cmd.size() == 1) {
            std::cout << "load: missing operand" << std::endl;
        } else {
            load(cmd[1]);
        }
    } else if (cmd[0] == "q") {
        std::cout << "QUIT" << std::endl;
        return 1;
    } else {
        std::cout << "UNKNOWN COMMAND" << std::endl;
    };
    return 0;
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
        std::string cmd_string = "";
        std::getline(std::cin, cmd_string);  // nacte cely radek
        if (cmd_string.size() > 0 && command(cmd_string)) {
            return 0;
        }
    }

}