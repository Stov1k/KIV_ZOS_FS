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
 * @param s1 nazev souboru
 */
pseudo_inode *getFileINode(filesystem &filesystem_data, std::string &s1);

#endif //ZOSFS_INODE_H
