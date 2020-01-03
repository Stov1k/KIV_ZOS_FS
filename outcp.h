//
// Created by pavel on 03.01.20.
//

#ifndef ZOSFS_OUTCP_H
#define ZOSFS_OUTCP_H

#include "zosfsstruct.h"

/**
 * Nahraje soubor s1 z pseudoNTFS do umisteni s2 na pevnem disku
 * @param filesystem_data filesystem
 * @param s1 soubor pseudoNTFS
 * @param s2 soubor na pevnem disku
 */
void outcp(filesystem &filesystem_data, std::string &s1, std::string &s2);

#endif //ZOSFS_OUTCP_H
