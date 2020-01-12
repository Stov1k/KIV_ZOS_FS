//
// Created by pavel on 09.01.20.
//

#ifndef ZOSFS_LS_H
#define ZOSFS_LS_H

#include "zosfsstruct.h"

/**
 * Vypise obsah adresare a1
 * @param filesystem_data filesystem
 * @param a1 adresar
 */
void ls(filesystem &filesystem_data, std::string &a1);

/**
 * Vypise obsah aktualniho adresare
 * @param filesystem_data filesystem
 */
void ls(filesystem &filesystem_data);

#endif //ZOSFS_LS_H
