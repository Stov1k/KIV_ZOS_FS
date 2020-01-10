//
// Created by pavel on 08.01.20.
//

#ifndef ZOSFS_MV_H
#define ZOSFS_MV_H

#include "zosfsstruct.h"

/**
 * Presune soubor s1 do umisteni s2
 * @param filesystem_data filesystem
 * @param s1 nazev souboru
 * @param s2 nazev souboru
 */
void mv(filesystem &filesystem_data, std::string &s1, std::string &s2);

/**
 * Zkopiruje soubor s1 do umisteni s2
 * @param filesystem_data filesystem
 * @param s1 nazev souboru
 * @param s2 nazev souboru
 */
void cp(filesystem &filesystem_data, std::string &s1, std::string &s2);

#endif //ZOSFS_MV_H
