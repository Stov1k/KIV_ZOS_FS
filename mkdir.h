//
// Created by pavel on 08.01.20.
//

#ifndef ZOSFS_MKDIR_H
#define ZOSFS_MKDIR_H

#include "zosfsstruct.h"

/**
 * Vytvori adresar a1
 * @param a1 nazev adresare
 */
void mkdir(filesystem &filesystem_data, std::string &a1);

#endif //ZOSFS_MKDIR_H
