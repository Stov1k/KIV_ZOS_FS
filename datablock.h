//
// Created by pavel on 29.12.19.
//

#ifndef ZOSFS_DATABLOCK_H
#define ZOSFS_DATABLOCK_H

#include "zosfsstruct.h"

/**
 * Pocet odkazu, ktere lze ulozit do jednoho databloku
 * @param filesystem_data filesystem
 * @return pocet odkazu
 */
int32_t linksPerCluster(filesystem &filesystem_data);

/**
 * Pocet dostupnych neobsazenych databloku na filesystemu
 * @param filesystem_data filesystem
 * @return pocet dostupnych neobsazenych databloku
 */
int32_t availableDatablocks(filesystem &filesystem_data);

/**
 * Vrati volnou pozici databloku
 * @param buf buffer - pozice v bloku, pozice v byte, bitmap byte, poradi bytu
 * @return vrati, zdali existuje volna pozice
 */
bool getFreeDatablock(filesystem &filesystem_data, int32_t buf[]);

/**
 * Vytvoreni datablocku neprimych odkazu
 * @param filesystem_data filesystem
 * @param links_per_cluster pocet odkazu na datablock
 * @param cpin_file otevreny soubor na pevnem disku
 * @param fs_file otevreny soubor filesystemu
 * @return pozice databloku
 */
int32_t createIndirectDatablock(filesystem &filesystem_data, std::fstream &fs_file);

/**
 * Vrati vector adres pouzitych databloku
 * @param filesystem_data filesystem
 * @param fs_file otevreny soubor filesystemu
 * @param inode adresar/soubor
 * @param structural_included vcetne strukturnich inodu (neprime odkazy)
 * @return vector adres databloku
 */
std::vector<int32_t> usedDatablockByINode(filesystem &filesystem_data, std::fstream &fs_file, pseudo_inode &inode, bool structural_included);

/**
 * Prida datablok
 * @param filesystem_data filesystem
 * @param fs_file otevreny soubor filesystemu
 * @param inode adresar/soubor
 * @return adresa databloku
 */
int32_t addDatablockToINode(filesystem &filesystem_data, std::fstream &fs_file, pseudo_inode &inode);

#endif //ZOSFS_DATABLOCK_H
