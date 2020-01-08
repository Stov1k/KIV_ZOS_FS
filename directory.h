//
// Created by pavel on 28.12.19.
//

#ifndef ZOSFS_DIRECTORY_H
#define ZOSFS_DIRECTORY_H

#include <cstring>
#include <bitset>
#include <vector>
#include "zosfsstruct.h"

/**
 * Rozdeleni cesty na adresare
 * @param path cesta
 * @return segments vektor adresaru
 */
std::vector<std::string> splitPath(const std::string path);

/**
 * Vypise adresare
 * @param directories vektor adresaru
 */
void printDirectories(const std::vector<directory_item> &directories);

/**
 * Vrati vektor podadresaru v aktualnim adresari
 * @return vektor adresaru
 */
std::vector<directory_item> getDirectories(filesystem &filesystem_data);

/**
 * Vrati, zdali adresar tehoz jmena jiz existuje
 * @param dir adresar
 * @return existuje adresar stejneho jmena?
 */
bool isDirectoryExists(filesystem &filesystem_data, directory_item &dir);

/**
 * Vrati referenci na adresar
 * @param nodeid cislo node
 * @param name nazev adresare
 * @return reference na adresar
 */
directory_item getDirectory(int32_t nodeid, std::string name);

/**
 * Vrati nadrazeny adresar
 * @param filesystem_data filesystem
 * @param fs_file otevreny soubor filesystemu
 * @param inode adresar
 * @return nadrazeny adresar
 */
pseudo_inode *getParrentDirectory(filesystem &filesystem_data, std::fstream &fs_file, pseudo_inode &inode);

#endif //ZOSFS_DIRECTORY_H
