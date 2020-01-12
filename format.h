//
// Created by pavel on 12.01.20.
//

#ifndef ZOSFS_FORMAT_H
#define ZOSFS_FORMAT_H

/**
 * Naformatovani souboru pseudoFS
 * @param filesystem_data filesystem
 * @param size velikost disku
 */
void format(filesystem &filesystem_data, int32_t size);

#endif //ZOSFS_FORMAT_H
