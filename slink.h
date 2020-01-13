//
// Created by pavel on 13.01.20.
//

#ifndef ZOSFS_SLINK_H
#define ZOSFS_SLINK_H

#include "zosfsstruct.h"

/**
 * Vytvori symbolicky link na soubor s1 s nazvem s2
 * @param filesystem_data filesystem
 * @param s1 nazev souboru
 * @param s2 nazev souboru
 */
void slink(filesystem &filesystem_data, std::string &s1, std::string &s2);

#endif //ZOSFS_SLINK_H
