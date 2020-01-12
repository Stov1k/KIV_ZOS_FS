//
// Created by pavel on 28.12.19.
//

#include <cstring>
#include <bitset>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include "zosfsstruct.h"
#include "inode.h"
#include "datablock.h"

/**
 * Pocet adresaru, ktere lze ulozit do jednoho databloku
 * @param filesystem_data filesystem
 * @return pocet adresaru
 */
int32_t dirsPerCluster(filesystem &filesystem_data) {
    return filesystem_data.super_block.cluster_size / sizeof(directory_item);
}

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
 * Vypise adresare
 * @param directories vektor adresaru
 */
void printDirectories(const std::vector<directory_item> &directories) {
    for (auto &directory : directories) {
        std::cout << "\t" << directory.inode << "\t" << directory.item_name << std::endl;
    }
}

/**
 * Vrati vektor podadresaru v aktualnim adresari
 * @param filesystem_data filesystem
 * @param working_dir pracovni adresar
 * @return vektor adresaru
 */
std::vector<directory_item> getDirectories(filesystem &filesystem_data, pseudo_inode &working_dir) {
    std::fstream fs_file;
    fs_file.open(filesystem_data.fs_file, std::ios::in);

    std::vector<directory_item> directories;

    // pocet adresaru na jeden data blok
    int32_t dirs_per_cluster = dirsPerCluster(filesystem_data);
    directory_item dirs_array[dirs_per_cluster];

    // platne adresy na databloky
    std::vector<int32_t> addresses = usedDatablockByINode(filesystem_data, fs_file, working_dir, false);

    // prochazeni adres databloku
    for (auto &address : addresses) {
        // nacteni slozek a vlozeni do vectoru
        fs_file.seekp(address);
        fs_file.read(reinterpret_cast<char *>(&dirs_array), sizeof(dirs_array));
        for (int i = 0; i < dirs_per_cluster; i++) {
            if (dirs_array[i].inode) {
                directories.push_back(dirs_array[i]);
            }
        }
    }

    fs_file.close();
    return directories;
}

/**
 * Smaze zaznam adresare/souboru directory do adresare working_dir
 * @param filesystem_data filesystem
 * @param working_dir pracovni adresar
 * @param directory zaznam adresare/souboru
 * @return usech? (adresa databloku, kde byl zaznam smazan)
 */
int32_t removeDirectoryItemEntry(filesystem &filesystem_data, pseudo_inode &working_dir, directory_item &directory) {
    // musi se jednat o existujici inode
    if(working_dir.nodeid == 0) {
        return 0;
    }

    std::fstream fs_file;
    fs_file.open(filesystem_data.fs_file, std::ios::in | std::ios::out | std::ios::binary);

    // pocet adresaru na jeden data blok
    int32_t dirs_per_cluster = dirsPerCluster(filesystem_data);
    directory_item directories[dirs_per_cluster];

    // platne adresy na databloky
    std::vector<int32_t> addresses = usedDatablockByINode(filesystem_data, fs_file, working_dir, false);

    // zmena na adrese
    int32_t record_address = 0;

    // prochazeni adres databloku
    for (auto &address : addresses) {
        // nacteni slozek
        fs_file.seekp(address);
        fs_file.read(reinterpret_cast<char *>(&directories), sizeof(directories));
        for (int i = 0; i < dirs_per_cluster; i++) {
            if (directories[i].inode == directory.inode) {
                if(strcmp(directories[i].item_name,directory.item_name) == 0) {
                    record_address = address;
                    directories[i].inode = 0;
                    std::strncpy(directories[i].item_name, "\0", sizeof(directories[i].item_name));
                    break;
                }
            }
        }
    }

    if(record_address) {
        fs_file.seekp(record_address);
        fs_file.write(reinterpret_cast<const char *>(&directories), sizeof(directories));
    }

    fs_file.close();
    return record_address;
}

/**
 * Prida zaznam adresare/souboru directory do adresare working_dir
 * @param filesystem_data filesystem
 * @param working_dir pracovni adresar
 * @param directory zaznam adresare/souboru
 * @return usech? (adresa databloku, kde je zaznam ulozen)
 */
int32_t addDirectoryItemEntry(filesystem &filesystem_data, pseudo_inode &working_dir, directory_item &directory) {
    // musi se jednat o existujici inode
    if(working_dir.nodeid == 0) {
        return 0;
    }

    std::fstream fs_file;
    fs_file.open(filesystem_data.fs_file, std::ios::in | std::ios::out | std::ios::binary);

    // pocet adresaru na jeden data blok
    int32_t dirs_per_cluster = dirsPerCluster(filesystem_data);
    directory_item directories[dirs_per_cluster];

    // platne adresy na databloky
    std::vector<int32_t> addresses = usedDatablockByINode(filesystem_data, fs_file, working_dir, false);

    // zaznamenano na adresu
    int32_t record_address = 0;

    // prochazeni adres databloku
    for (auto &address : addresses) {
        // nacteni slozek
        fs_file.seekp(address);
        fs_file.read(reinterpret_cast<char *>(&directories), sizeof(directories));
        for (int i = 0; i < dirs_per_cluster; i++) {
            if (!directories[i].inode) {
                directories[i].inode = directory.inode;
                std::strncpy(directories[i].item_name, directory.item_name, sizeof(directories[i].item_name));
                record_address = address;
            }
            if(record_address) break;
        }
        if(record_address) break;
    }

    // pokusi se obdrzet novy datablok
    if(!record_address) {
        int32_t obtained_address = addDatablockToINode(filesystem_data,fs_file,working_dir);
        if(obtained_address) {
            for (int i = 0; i < dirs_per_cluster; i++) {
                directories[i] = directory_item{};
                directories[i].inode = 0;
            }
            directories[0].inode = directory.inode;
            std::strncpy(directories[0].item_name, directory.item_name, sizeof(directories[0].item_name));
            record_address = obtained_address;
        }
    }

    if(record_address) {
        fs_file.seekp(record_address);
        fs_file.write(reinterpret_cast<const char *>(&directories), sizeof(directories));
    }

    fs_file.close();

    return record_address;
}

/**
 * Vrati referenci na adresar
 * @param nodeid cislo node
 * @param name nazev adresare
 * @return reference na adresar
 */
directory_item createDirectoryItem(int32_t nodeid, std::string name) {
    directory_item dir = directory_item{};
    dir.inode = nodeid;
    std::strncpy(dir.item_name, name.c_str(), sizeof(dir.item_name));
    dir.item_name[sizeof(dir.item_name) - 1] = '\0';
    return dir;
}

/**
 * Vrati referenci na adresar
 * @param filesystem_data filesystem
 * @param working_dir pracovni adresar
 * @param name nezev adresare
 * @return reference na adresar
 */
directory_item * getDirectoryItem(filesystem &filesystem_data, pseudo_inode &working_dir, std::string name) {
    directory_item * dir_ptr = nullptr;
    directory_item dir_name = createDirectoryItem(0, name);
    std::vector<directory_item> directories = getDirectories(filesystem_data, working_dir);
    for (auto &directory : directories) {
        directory.item_name;
        if (strcmp(dir_name.item_name, directory.item_name) == 0) {
            dir_ptr = &directory;
            break;
        }
    }
    return dir_ptr;
}


/**
 * Vrati, zdali adresar tehoz jmena jiz existuje
 * @param filesystem_data filesystem
 * @param working_dir pracovni adresar
 * @param name nezev adresare
 * @return existuje adresar stejneho jmena?
 */
bool isDirectoryExists(filesystem &filesystem_data, pseudo_inode &working_dir, std::string name) {
    directory_item * dir_ptr = getDirectoryItem(filesystem_data, working_dir, name);
    if(dir_ptr != nullptr) {
        return true;
    }
    return false;
}

/**
 * Vrati nadrazeny adresar
 * @param filesystem_data filesystem
 * @param fs_file otevreny soubor filesystemu
 * @param inode adresar
 * @return nadrazeny adresar
 */
pseudo_inode *getParrentDirectory(filesystem &filesystem_data, std::fstream &fs_file, pseudo_inode &inode) {

    uint32_t dirs_per_cluster = filesystem_data.super_block.cluster_size / sizeof(directory_item);
    directory_item directories[dirs_per_cluster];

    pseudo_inode parrent;
    pseudo_inode *parrent_ptr = nullptr;

    if (inode.direct1 != 0) {
        fs_file.seekp(inode.direct1);
        fs_file.read(reinterpret_cast<char *>(&directories), sizeof(directories));
        fs_file.seekp(getINodePosition(filesystem_data, directories[1].inode));
        fs_file.read(reinterpret_cast<char *>(&parrent), sizeof(parrent));
        parrent_ptr = &parrent;
    }

    return parrent_ptr;
}