//
// Created by pavel on 06.01.20.
//

#ifndef ZOSFS_PWD_H
#define ZOSFS_PWD_H

#include "zosfsstruct.h"

/**
 * Vypise cestu
 * @param filesystem_data filesystem
 * @param inode adresar
 * @param verbose vypise cestu
 * @return cesta
 */
std::string pwd(filesystem &filesystem_data, pseudo_inode &inode, bool verbose);

/**
 * Vypise aktualni cestu
 * @param filesystem_data filesystem
 * @return cesta
 */
std::string pwd(filesystem &filesystem_data);

#endif //ZOSFS_PWD_H
