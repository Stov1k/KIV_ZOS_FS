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
#include <sys/stat.h>
#include <cstdio>

filesystem filesystem_data;

/**
 * Vypise adresare
 * @param directories vektor adresaru
 */
void printDirectories(const std::vector<directory_item>& directories) {
    for(auto& directory : directories) {
        std::cout << "\t"<< directory.inode << "\t" << directory.item_name << std::endl;
    }
}

/**
 * Vrati referenci na adresar
 * @param nodeid cislo node
 * @param name nazev adresare
 * @return reference na adresar
 */
directory_item getDirectory(int32_t nodeid, std::string name) {
    directory_item dir = directory_item{};
    dir.inode = nodeid;
    std::strncpy(dir.item_name, name.c_str(), sizeof(dir.item_name));
    dir.item_name[sizeof(dir.item_name) - 1] = '\0';
    return dir;
}

/**
 * Vrati vektor podadresaru v aktualnim adresari
 * @return vektor adresaru
 */
std::vector<directory_item> getDirectories() {
    std::fstream input_file;
    input_file.open(filesystem_data.fs_file, std::ios::in);

    std::vector<directory_item> directories;

    // pocet adresaru na jeden data blok
    uint32_t dirs_per_cluster = filesystem_data.super_block.cluster_size / sizeof(directory_item);

    // skok na prvni primy odkaz v inode (TODO: pozdeji predelat na iteraci)
    input_file.seekp(filesystem_data.current_dir.direct1);

    // nacteni slozek a vlozeni do vectoru
    directory_item dirs_array[dirs_per_cluster];
    input_file.read(reinterpret_cast<char *>(&dirs_array), sizeof(dirs_array));
    for(int i = 0; i < dirs_per_cluster; i++) {
        if (dirs_array[i].inode) {
            directories.push_back(dirs_array[i]);
        }
    }
    input_file.close();
    return directories;
}

/**
 * Vrati, zdali adresar tehoz jmena jiz existuje
 * @param dir adresar
 * @return existuje adresar stejneho jmena?
 */
bool isDirectoryExists(directory_item& dir) {
    std::vector<directory_item> directories = getDirectories();
    for(auto& directory : directories) {
        directory.item_name;
        if(strcmp(dir.item_name,directory.item_name) == 0) {
            return true;
        }
    }
    return false;
}

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

        int bitmap_size_bytes = (filesystem_data.super_block.inode_start_address - filesystem_data.super_block.bitmap_start_address);

        for(int i = 0; i < bitmap_size_bytes; i++) {
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
    input_file.read(reinterpret_cast<char *>(&filesystem_data.current_dir), sizeof(pseudo_inode));

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
    std::strncpy(filesystem_data.super_block.signature, signature.c_str(), sizeof(filesystem_data.super_block.signature));
    filesystem_data.super_block.signature[sizeof(filesystem_data.super_block.signature) - 1] = '\0';
    std::strncpy(filesystem_data.super_block.volume_descriptor, description.c_str(), sizeof(filesystem_data.super_block.volume_descriptor));
    filesystem_data.super_block.volume_descriptor[sizeof(filesystem_data.super_block.volume_descriptor) - 1] = '\0';

    int cluster_size = 1024;
    remaining_space = remaining_space - sizeof(superblock);
    int inodes = remaining_space/cluster_size;
    remaining_space = remaining_space - sizeof(pseudo_inode);
    int bitmap_size = remaining_space/cluster_size;
    int correction = bitmap_size%8;                     // velikost FS bude vzdy delitelna 8 (bitmapa pak vyuzije vzdy cely byte)
    bitmap_size = bitmap_size - correction;
    int bitmap_size_bytes = bitmap_size/8;

    int clusters = bitmap_size;

    filesystem_data.super_block.disk_size = sizeof(superblock) + bitmap_size + inodes + cluster_size*clusters;
    filesystem_data.super_block.cluster_size = cluster_size;
    filesystem_data.super_block.cluster_count = clusters;
    filesystem_data.super_block.bitmap_start_address = sizeof(filesystem_data.super_block);
    filesystem_data.super_block.inode_start_address = sizeof(filesystem_data.super_block) + sizeof(byte)*bitmap_size_bytes;
    filesystem_data.super_block.data_start_address = sizeof(filesystem_data.super_block) + sizeof(byte)*bitmap_size_bytes + sizeof(pseudo_inode)*inodes;
    input_file.write(reinterpret_cast<const char *>(&filesystem_data.super_block), sizeof(superblock));


    // zapis inodu
    pseudo_inode inode;
    for(int i = 0; i < inodes; i++) {
        inode = pseudo_inode{};
        inode.nodeid = ID_ITEM_FREE;
        input_file.write(reinterpret_cast<const char *>(&inode), sizeof(pseudo_inode));
    }

    // zapis bitmapy
    for (int bmp = 0; bmp < bitmap_size_bytes; bmp++) {
        std::cout << (int)byte << " pos:" << input_file.tellg() << std::endl;
        input_file.write(reinterpret_cast<const char *>(&byte), sizeof(byte));
    }

    // zapis vsech clusteru (data bloku)
    for(int i = 0; i < clusters; i++) {
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

    // aktualni adresar
    dir = getDirectory(inode.nodeid, ".");

    int32_t dirs_per_cluster = cluster_size / sizeof(directory_item);

    // predchozi adresar
    directory_item dir_parrent;
    dir_parrent = getDirectory(inode.nodeid, "..");;

    // nastaveni referenci na adresare
    directory_item directories[dirs_per_cluster];
    directories[0] = dir;                           // .
    directories[1] = dir_parrent;                   // ..
    for(int i = 2; i < dirs_per_cluster; i++) {     // dalsi neobsazene
        directories[i] = directory_item{};
        directories[i].inode = 0;   // tj. nic
    }
    filesystem_data.current_dir = inode;

    // vytvoreni korenoveho adresare: uprava bitmapy
    input_file.seekp(filesystem_data.super_block.bitmap_start_address); // skoci na bitmapu
    uint8_t byte1{1};       // TODO: neni idealni, chci menit jen 1 bit, ne 1 byte
    std::cout << (int)byte1 << " pos:" << input_file.tellg() << std::endl;
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
 * Vrati volnou pozici databloku
 * @param buf buffer - pozice v bloku, pozice v byte, bitmap byte
 * @return vrati, zdali existuje volna pozice
 */
bool getFreePosition(int32_t buf[]) {
    std::fstream input_file;
    input_file.open(filesystem_data.fs_file, std::ios::in);

    input_file.seekp(filesystem_data.super_block.bitmap_start_address);
    bool found = false;
    uint8_t bitmap_byte;
    int bitmap_size_bytes = (filesystem_data.super_block.inode_start_address - filesystem_data.super_block.bitmap_start_address);
    for(int i = 0; i < bitmap_size_bytes; i++) {
        char b;
        input_file.read(&b, 1);

        // bitova mnozina reprezentujici 8 databloku
        std::bitset<8> x(b);

        // vyhledani nuloveho bitu
        for(int i = 0; i < 8; i++) {
            bool used = x.test(i);
            if(!found && !used) {
                found = true;
                buf[1] = i;
            }
            std::cout << i << " " << used << std::endl;
        }
        std::cout << "Pozice " << buf[1] << std::endl;

        // zmena bitu
        if(found) {
            x.set(buf[1], true);   // zmeni cislo na pozici na 1
        }
        unsigned long ul = x.to_ulong();
        unsigned int c = static_cast<unsigned int>(ul);
        bitmap_byte = c;
        std::cout << x << " pos:" << input_file.tellg() << " Long:" << ul << " Int:" << c << std::endl;

        // konec cyklu
        if(found) {
            buf[0] = buf[1] + (8*i);
            buf[2] = bitmap_byte;
            break;
        }
    }

    input_file.close();
    if(found) {
        return true;
    }
    return false;
}

/**
 * Prikaz mkdir
 * @param dir_name nazev adresare
 */
void mkdir(std::string &dir_name) {

    std::fstream input_file;
    input_file.open(filesystem_data.fs_file, std::ios::in | std::ios::out | std::ios::binary);

    // novy adresar
    directory_item dir = getDirectory(0, dir_name);

    // nalezeny volny inode
    pseudo_inode inode;
    pseudo_inode * inode_ptr;

    // zjistim, zdali jiz neexistuje adresar stejneho nazvu
    if(isDirectoryExists(dir)) {
        input_file.close();
        std::cout << "EXIST" << std::endl;
        return;
    }

    // najdu volny inode
    inode_ptr = getFreeINode(filesystem_data);
    if(inode_ptr != nullptr) {
        inode = *inode_ptr;
    } else {
        input_file.close();
        std::cout << "FREE INODE NOT FOUND" << std::endl;
        return;
    }

    // najdu v bitmape volny datablok
    int32_t buf[3];
    bool found = getFreePosition(buf);
    if(!found) {
        std::cout << "DISK IS FULL" << std::endl;
        return;
    }
    int32_t position = buf[0];
    uint8_t bitmap_byte = (uint8_t) buf[2];

    //v korenovem adresari vytvorim polozku dir(nazev, inode)
    input_file.seekp(filesystem_data.current_dir.direct1);
    uint32_t dirs_per_cluster = filesystem_data.super_block.cluster_size / sizeof(directory_item);
    directory_item dirs[dirs_per_cluster];
    input_file.read(reinterpret_cast<char *>(&dirs), sizeof(dirs));
    for(int i = 0; i < dirs_per_cluster; i++) {
        if(!dirs[i].inode) {
            std::strncpy(dirs[i].item_name, dir.item_name, sizeof(dir.item_name));
            dirs[i].inode = inode.nodeid;
            break;
        }
    }

    // aktualizace bitmapy
    input_file.seekp(filesystem_data.super_block.bitmap_start_address); // skoci na bitmapu
    input_file.write(reinterpret_cast<const char *>(&bitmap_byte), sizeof(bitmap_byte));    // zapise upravenou bitmapu

    // aktualizace podslozek v nadrazenem adresari
    input_file.seekp(filesystem_data.current_dir.direct1); // skoci na data
    input_file.write(reinterpret_cast<const char *>(&dirs), sizeof(dirs));

    // zapis inode
    input_file.seekp(filesystem_data.super_block.inode_start_address + (inode.nodeid-1) * sizeof(pseudo_inode));
    inode.isDirectory = true;
    inode.references++;
    inode.direct1 = filesystem_data.super_block.data_start_address + (position * filesystem_data.super_block.cluster_size);
    inode.direct2 = 0;
    inode.direct3 = 0;
    inode.direct4 = 0;
    inode.direct5 = 0;
    inode.indirect1 = 0;
    inode.indirect2 = 0;
    input_file.write(reinterpret_cast<const char *>(&inode), sizeof(pseudo_inode));

    // aktualni adresar
    directory_item dir_dot;
    dir_dot = getDirectory(inode.nodeid, ".");

    // predchozi adresar
    directory_item dir_parrent;
    dir_parrent = getDirectory(filesystem_data.current_dir.nodeid, "..");

    // predchozi adresar
    directory_item dir_b;
    dir_b = getDirectory(filesystem_data.current_dir.nodeid, "b");

    directory_item subdirectories[dirs_per_cluster];
    subdirectories[0] = dir_dot;                       // .
    subdirectories[1] = dir_parrent;                   // ..
    for(int i = 2; i < dirs_per_cluster; i++) {        // dalsi neobsazene
        subdirectories[i] = directory_item{};
        subdirectories[i].inode = 0;   // tj. nic
    }

    // aktualizace podslozek v novem adresari
    input_file.seekp(filesystem_data.super_block.data_start_address + (position * filesystem_data.super_block.cluster_size)); // skoci na data
    input_file.write(reinterpret_cast<const char *>(&subdirectories), sizeof(subdirectories));

    input_file.close();
}

/**
 * Prikaz cd
 * @param dir_name nazev adresare
 */
void cd(std::string &dir_name) {
    std::fstream input_file;
    input_file.open(filesystem_data.fs_file, std::ios::in | std::ios::out | std::ios::binary);

    // zjistim, zdali existuje adresar stejneho nazvu
    std::vector<directory_item> directories = getDirectories();
    for(auto& directory : directories) {
        directory.item_name;
        if(strcmp(dir_name.c_str(),directory.item_name) == 0) {
            input_file.seekp(filesystem_data.super_block.inode_start_address + (directory.inode-1) * sizeof(pseudo_inode));
            input_file.read(reinterpret_cast<char *>(&filesystem_data.current_dir), sizeof(pseudo_inode));
            std::cout << "OK" << std::endl;
            break;
        }
    }

    input_file.close();
}

/**
 * Prikaz ls: vypise obsah aktualniho adresare
 */
void ls() {
    std::fstream input_file;
    input_file.open(filesystem_data.fs_file, std::ios::in);
    if (!input_file.is_open()) {
        std::cout << "Nelze otevrit." << std::endl;
    } else {
        // skok na inode
        input_file.seekp(filesystem_data.current_dir.direct1);

        uint32_t dirs_per_cluster = filesystem_data.super_block.cluster_size / sizeof(directory_item);
        directory_item directories[dirs_per_cluster];

        input_file.read(reinterpret_cast<char *>(&directories), sizeof(directories));
        for(int i = 0; i < dirs_per_cluster; i++) {
            if(directories[i].inode) {
                std::cout << i << "\t"<< directories[i].inode << "\t" << directories[i].item_name << std::endl;
            }
        }
    }
    input_file.close();
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
    if(argc >= 2) {
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

    std::vector<directory_item> directories = getDirectories();
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
                    if(isdigit(cmd[1].at(i))) {
                        str_size.push_back(cmd[1].at(i));
                    } else {
                        fs_size = std::stoi(str_size);
                        if(cmd[1].at(i) == 'M') {
                            fs_size = fs_size * 1048576;
                            std::cout << "MB" << std::endl;
                        } else if(cmd[1].at(i) == 'K') {
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
        }  else if (input_string == "bitmap") {
            std::cout << "Bitmapa... " << std::endl;
            print_bitmap();
        }  else if (cmd.size() > 0 && cmd[0] == "ls") {
            int bitmap_size_bytes = (filesystem_data.super_block.inode_start_address - filesystem_data.super_block.bitmap_start_address);
            std::cout << "Obsah " << bitmap_size_bytes << std::endl;
            ls();
        }  else if (cmd.size() > 0 && cmd[0] == "mkdir") {
            if(cmd.size() == 1) {
                std::cout << "CANNOT CREATE DIRECTORY" << std::endl;
            } else {
                mkdir(cmd[1]);
            }
        }  else if (cmd.size() > 0 && cmd[0] == "incp") {
            if(cmd.size() < 3) {
                std::cout << "PATH NOT FOUND" << std::endl;
            } else {
                inputCopy(filesystem_data, cmd[1], cmd[1]);
            }
        }  else if (cmd.size() > 0 && cmd[0] == "cd") {
            if(cmd.size() == 1) {
                std::cout << "CANNOT OPEN DIRECTORY" << std::endl;
            } else {
                cd(cmd[1]);
            }
        } else if (input_string == "q") {
            std::cout << "Konec" << std::endl;
            return 0;
        };
    }

    return 0;
}