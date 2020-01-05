//
// Created by pavel on 05.01.20.
//

#ifndef ZOSFS_RM_H
#define ZOSFS_RM_H

#include "zosfsstruct.h"

/**
 * Smaze prazdny adresar a1
 * @param filesystem_data fileszystem
 * @param a1 nazev adresare
 */
void rmdir(filesystem &filesystem_data, std::string &a1);

/**
 * Smaze soubor s1
 * @param filesystem_data fileszystem
 * @param s1 nazev souboru
 */
void rm(filesystem &filesystem_data, std::string &a1);

#endif //ZOSFS_RM_H
