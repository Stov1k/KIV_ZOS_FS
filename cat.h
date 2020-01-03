//
// Created by pavel on 03.01.20.
//

#ifndef ZOSFS_CAT_H
#define ZOSFS_CAT_H

#include "zosfsstruct.h"

/**
 * Vypise obsah souboru s1
 * @param filesystem_data filesystem
 * @param s1 soubor
 */
void cat(filesystem &filesystem_data, std::string &s1);

#endif //ZOSFS_CAT_H
