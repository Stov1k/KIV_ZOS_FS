//
// Created by pavel on 12.01.20.
//

#ifndef ZOSFS_CP_H
#define ZOSFS_CP_H

/**
 * Zkopireje zdrojovy datablok do ciloveho databloku
 * @param filesystem_data informace filesystemu
 * @param fs_file otevreny soubor na pseudoNTFS
 * @param source_address pocatecni pozice zdrojoveho datablocku
 * @param target_address pocatecni pozice ciloveho datablocku
 */
void copyDataBlock(filesystem &filesystem_data, std::fstream &fs_file, int32_t source_address, int32_t target_address);

/**
 * Zkopiruje soubor s1 do umisteni s2
 * @param filesystem_data filesystem
 * @param s1 nazev souboru
 * @param s2 nazev souboru
 */
void cp(filesystem &filesystem_data, std::string &s1, std::string &s2);

#endif //ZOSFS_CP_H
