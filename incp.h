//
// Created by pavel on 28.12.19.
//

#ifndef ZOSFS_INCP_H
#define ZOSFS_INCP_H

#include "zosfsstruct.h"

/**
 * Nahraje soubor s1 z pevneho disku do umisteni s2 v pseudoFS
 * @param filesystem_data filesystem
 * @param s1 vstupni soubor na pevnem disku
 * @param s2 umisteni na pseudoFS
 */
void incp(filesystem &filesystem_data, std::string &s1, std::string &s2);

#endif //ZOSFS_INCP_H
