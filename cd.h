//
// Created by pavel on 07.01.20.
//

#ifndef ZOSFS_CD_H
#define ZOSFS_CD_H

/**
 * Zmeni aktualni cestu do adresare a1
 * @param filesystem_data filesystem
 * @param a1 nazev adresare
 */
void cd(filesystem &filesystem_data, std::string &a1);

#endif //ZOSFS_CD_H
