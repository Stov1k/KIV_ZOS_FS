//
// Created by pavel on 28.12.19.
//

#ifndef ZOSFS_INODE_H
#define ZOSFS_INODE_H

#include "zosfsstruct.h"

/**
 * Vrati volny inode
 * @return inode
 */
pseudo_inode * getFreeINode(filesystem &filesystem_data);

#endif //ZOSFS_INODE_H
