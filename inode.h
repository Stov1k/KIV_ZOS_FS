//
// Created by pavel on 28.12.19.
//

#ifndef ZOSFS_INODE_H
#define ZOSFS_INODE_H

#include "zosfsstruct.h"

/**
 * Vrati pozici inodu v souboru podle poradi inodu
 * @param filesystem_data filesystem
 * @param inode_no poradi inodu
 * @return pozice v souboru fs
 */
int32_t getINodePosition(filesystem &filesystem_data, int32_t inode_no);

/**
 * Vrati volny inode
 * @return inode
 */
pseudo_inode *getFreeINode(filesystem &filesystem_data);

/**
 * Vrati referenci na inode souboru
 * @param filesystem_data filesystem
 * @param working_dir pracovni adresar
 * @param s1 nazev souboru
 * @param verbose vypisovani zprav
 * reference na inode souboru
 */
pseudo_inode *getFileINode(filesystem &filesystem_data, pseudo_inode &working_dir, std::string &s1, bool verbose);

/**
 * Vrati referenci na inode adresare nebo souboru
 * @param filesystem_data filesystem
 * @param location lokace / nazev souboru
 * @param verbose vypisovani zprav
 */
pseudo_inode *iNodeByLocation(filesystem &filesystem_data, std::string &location, bool verbose);

/**
 * Uvolni inode
 * @param filesystem_data filesystem
 * @param fs_file otevreny soubor filesystemu
 * @param inode adresar
 */
void removeINode(filesystem &filesystem_data, std::fstream &fs_file, pseudo_inode &inode);

#endif //ZOSFS_INODE_H
