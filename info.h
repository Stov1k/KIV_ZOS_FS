//
// Created by pavel on 06.01.20.
//

#ifndef ZOSFS_INFO_H
#define ZOSFS_INFO_H

#include "zosfsstruct.h"

/**
 * Vypise informace o souboru/adresari s1/a1
 * @param filesystem_data fileszystem
 * @param name nazev
 */
void info(filesystem &filesystem_data, std::string &name);

#endif //ZOSFS_INFO_H
