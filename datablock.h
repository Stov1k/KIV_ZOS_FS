//
// Created by pavel on 29.12.19.
//

#ifndef ZOSFS_DATABLOCK_H
#define ZOSFS_DATABLOCK_H

#include "zosfsstruct.h"

/**
 * Vrati volnou pozici databloku
 * @param buf buffer - pozice v bloku, pozice v byte, bitmap byte, poradi bytu
 * @return vrati, zdali existuje volna pozice
 */
bool getFreeDatablock(filesystem &filesystem_data, int32_t buf[]);

#endif //ZOSFS_DATABLOCK_H
