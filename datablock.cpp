//
// Created by pavel on 29.12.19.
//

#include <cstdint>
#include <fstream>
#include <bitset>
#include <iostream>
#include <vector>
#include "datablock.h"
#include "zosfsstruct.h"
#include "inode.h"

/**
 * Pocet odkazu, ktere lze ulozit do jednoho databloku
 * @param filesystem_data filesystem
 * @return pocet odkazu
 */
int32_t linksPerCluster(filesystem &filesystem_data) {
    return filesystem_data.super_block.cluster_size / sizeof(int32_t);
}

/**
 * Maximalni pocet databloku jednoho inodu
 * @param filesystem_data filesystem
 * @return max pocet databloku jednoho inodu
 */
int32_t maximumDatablocksPerINode(filesystem &filesystem_data) {
    int32_t links_per_cluster = linksPerCluster(filesystem_data);
    return 5 + links_per_cluster + links_per_cluster*links_per_cluster;
}

/**
 * Pocet dostupnych neobsazenych databloku na filesystemu
 * @param filesystem_data filesystem
 * @return pocet dostupnych neobsazenych databloku
 */
int32_t availableDatablocks(filesystem &filesystem_data) {
    int32_t avaible_datablocks = 0;
    std::fstream input_file;
    input_file.open(filesystem_data.fs_file, std::ios::in);
    // presun na pocatek bitmapy
    input_file.seekp(filesystem_data.super_block.bitmap_start_address);
    // velikost bitmapy v bytech
    int bitmap_size_bytes = (filesystem_data.super_block.inode_start_address -
                             filesystem_data.super_block.bitmap_start_address);
    // prochazeni bitmapy
    for (int i = 0; i < bitmap_size_bytes; i++) {
        char b;
        input_file.read(&b, 1);
        // bitova mnozina reprezentujici 8 databloku
        std::bitset<8> x(b);
        // vyhledani nuloveho bitu
        for (int j = 0; j < 8; j++) {
            bool used = x.test(j);
            if (!used) {
                avaible_datablocks = avaible_datablocks + 1;
            }
        }
    }
    input_file.close();
    return avaible_datablocks;
}

/**
 * Vrati volnou pozici databloku
 * @param buf buffer - pozice v bloku, pozice v byte, bitmap byte, poradi bytu
 * @return vrati, zdali existuje volna pozice
 */
bool getFreeDatablock(filesystem &filesystem_data, int32_t buf[]) {
    std::fstream input_file;
    input_file.open(filesystem_data.fs_file, std::ios::in);

    input_file.seekp(filesystem_data.super_block.bitmap_start_address);
    bool found = false;
    uint8_t bitmap_byte;
    int bitmap_size_bytes = (filesystem_data.super_block.inode_start_address -
                             filesystem_data.super_block.bitmap_start_address);
    for (int i = 0; i < bitmap_size_bytes; i++) {
        char b;
        input_file.read(&b, 1);

        // bitova mnozina reprezentujici 8 databloku
        std::bitset<8> x(b);

        // vyhledani nuloveho bitu
        for (int j = 0; j < 8; j++) {
            bool used = x.test(j);
            if (!found && !used) {
                found = true;
                buf[1] = j;
            }
        }

        // zmena bitu
        if (found) {
            x.set(buf[1], true);   // zmeni cislo na pozici na 1
        }
        unsigned long ul = x.to_ulong();
        unsigned int c = static_cast<unsigned int>(ul);
        bitmap_byte = c;

        // konec cyklu
        if (found) {
            buf[0] = buf[1] + (8 * i);
            buf[2] = bitmap_byte;
            buf[3] = i;
            break;
        }
    }

    input_file.close();
    if (found) {
        return true;
    }
    return false;
}

/**
 * Vytvoreni datablocku neprimych odkazu
 * @param filesystem_data filesystem
 * @param links_per_cluster pocet odkazu na datablock
 * @param cpin_file otevreny soubor na pevnem disku
 * @param fs_file otevreny soubor filesystemu
 * @return pozice databloku
 */
int32_t createIndirectDatablock(filesystem &filesystem_data, std::fstream &fs_file) {
    int32_t links_per_cluster = linksPerCluster(filesystem_data);
    int32_t links[links_per_cluster];
    for (int i = 0; i < links_per_cluster; i++) {
        links[i] = 0;
    }

    // ziskani datablocku (odkazy)
    int32_t datablock[4];
    getFreeDatablock(filesystem_data, datablock);
    int32_t position_byte = datablock[3];           // poradi bytu v bitmape
    uint8_t bitmap_byte = (uint8_t) datablock[2];

    // aktualizace bitmapy (odkazy)
    fs_file.seekp(filesystem_data.super_block.bitmap_start_address + position_byte); // skoci na bitmapu
    fs_file.write(reinterpret_cast<const char *>(&bitmap_byte), sizeof(bitmap_byte));    // zapise upravenou bitmapu

    // aktualizace dat (odkazy)
    int32_t datablock_position =
            filesystem_data.super_block.data_start_address + (datablock[0] * filesystem_data.super_block.cluster_size);
    fs_file.seekp(datablock_position);
    fs_file.write(reinterpret_cast<const char *>(&links), filesystem_data.super_block.cluster_size);

    return datablock_position;
}

/**
 * Vrati vector adres pouzitych databloku
 * @param filesystem_data filesystem
 * @param fs_file otevreny soubor filesystemu
 * @param inode adresar/soubor
 * @param structural_included vcetne strukturnich inodu (neprime odkazy)
 * @return vector adres databloku
 */
std::vector<int32_t> usedDatablockByINode(filesystem &filesystem_data, std::fstream &fs_file, pseudo_inode &inode, bool structural_included) {
    std::vector<int32_t> addresses;
    if (inode.direct1) addresses.push_back(inode.direct1);
    if (inode.direct2) addresses.push_back(inode.direct2);
    if (inode.direct3) addresses.push_back(inode.direct3);
    if (inode.direct4) addresses.push_back(inode.direct4);
    if (inode.direct5) addresses.push_back(inode.direct5);
    if (inode.indirect1 || inode.indirect1) {
        int32_t links_per_cluster = linksPerCluster(filesystem_data);
        int32_t links[links_per_cluster];
        if (inode.indirect1) {
            if(structural_included) addresses.push_back(inode.indirect1);
            fs_file.seekp(inode.indirect1);
            fs_file.read(reinterpret_cast<char *>(&links), sizeof(links));
            for (int i = 0; i < links_per_cluster; i++) {
                if (links[i]) {
                    addresses.push_back(links[i]);
                }
            }
        }
        if (inode.indirect2) {
            if(structural_included) addresses.push_back(inode.indirect2);
            int32_t sublinks[links_per_cluster];
            fs_file.seekp(inode.indirect2);
            fs_file.read(reinterpret_cast<char *>(&links), sizeof(links));
            for (int i = 0; i < links_per_cluster; i++) {
                if (links[i]) {
                    if(structural_included) addresses.push_back(links[i]);
                    fs_file.seekp(links[i]);
                    fs_file.read(reinterpret_cast<char *>(&sublinks), sizeof(sublinks));
                    for (int j = 0; j < links_per_cluster; j++) {
                        if (sublinks[j]) addresses.push_back(sublinks[j]);
                    }
                };
            }
        }
    }

    return addresses;
}

/**
 * Zabere datablok
 * @param filesystem_data filesystem
 * @return pozice databloku
 */
int32_t castDatablock(filesystem &filesystem_data) {
    int32_t datablock_position = 0;
    int32_t datablock[4];
    bool isFree = getFreeDatablock(filesystem_data, datablock);
    if(isFree) {
        datablock_position = filesystem_data.super_block.data_start_address +
                             (datablock[0] * filesystem_data.super_block.cluster_size);
    }
    return datablock_position;
}

/**
 * Prida datablok
 * @param filesystem_data filesystem
 * @param fs_file otevreny soubor filesystemu
 * @param inode adresar/soubor
 * @return adresa databloku
 */
int32_t addDatablockToINode(filesystem &filesystem_data, std::fstream &fs_file, pseudo_inode &inode) {
    // musi se jednat o existujici inode
    if(inode.nodeid == 0) {
        return 0;
    }
    // dostupne zdroje
    int32_t datablock_position = 0;
    int32_t avaible_datablocks = availableDatablocks(filesystem_data);
    int32_t maximum_datablocks_per_inode = maximumDatablocksPerINode(filesystem_data);
    int32_t used_datablocks_by_inode = 0;
    // platne adresy na databloky
    std::vector<int32_t> addresses = usedDatablockByINode(filesystem_data, fs_file, inode, false);
    // prochazeni adres databloku
    for (auto &address : addresses) {
        used_datablocks_by_inode++;
    }
    int32_t avaible_datablocks_by_inode = maximum_datablocks_per_inode-used_datablocks_by_inode;
    int32_t avaible = std::max(avaible_datablocks,avaible_datablocks_by_inode);

    if(avaible) {
        if (!inode.direct1) {
            inode.direct1 = castDatablock(filesystem_data);
            datablock_position = inode.direct1;
        } else if (!inode.direct2) {
            inode.direct2 = castDatablock(filesystem_data);
            datablock_position = inode.direct2;
        } else if (!inode.direct3) {
            inode.direct3 = castDatablock(filesystem_data);
            datablock_position = inode.direct3;
        } else if (!inode.direct4) {
            inode.direct4 = castDatablock(filesystem_data);
            datablock_position = inode.direct4;
        } else if (!inode.direct5) {
            inode.direct5 = castDatablock(filesystem_data);
            datablock_position = inode.direct5;
        } else if (!inode.indirect1) {
            inode.indirect1 = createIndirectDatablock(filesystem_data, fs_file);
            avaible = avaible-1;
        }
        // indirect 1
        if(!datablock_position && avaible) {
            int32_t links_per_cluster = linksPerCluster(filesystem_data);
            int32_t links[links_per_cluster];
            fs_file.seekp(inode.indirect1);
            fs_file.read(reinterpret_cast<char *>(&links), sizeof(links));
            for (int i = 0; i < links_per_cluster; i++) {
                if (!links[i]) {
                    links[i] = castDatablock(filesystem_data);
                    datablock_position = links[i];
                    // aktualizace odkazu
                    fs_file.seekp(inode.indirect1);
                    fs_file.write(reinterpret_cast<const char *>(&links), filesystem_data.super_block.cluster_size);
                    break;
                }
            }
            // indirect 2
            if(!datablock_position && avaible) {
                int32_t sublinks[links_per_cluster];
                if (!inode.indirect2) {
                    inode.indirect2 = createIndirectDatablock(filesystem_data, fs_file);
                    avaible = avaible - 1;
                }
                if(avaible) {
                    fs_file.seekp(inode.indirect2);
                    fs_file.read(reinterpret_cast<char *>(&links), sizeof(links));
                    for (int i = 0; i < links_per_cluster; i++) {
                        if (links[i]) {
                            fs_file.seekp(links[i]);
                            fs_file.read(reinterpret_cast<char *>(&sublinks), sizeof(sublinks));
                            for (int j = 0; j < links_per_cluster; j++) {
                                if(!sublinks[j]) {
                                    sublinks[j] = castDatablock(filesystem_data);
                                    datablock_position = sublinks[j];
                                    // aktualizace odkazu
                                    fs_file.seekp(links[i]);
                                    fs_file.write(reinterpret_cast<const char *>(&links), filesystem_data.super_block.cluster_size);
                                    break;
                                }
                            }
                        } else {
                            links[i] = createIndirectDatablock(filesystem_data, fs_file);
                            avaible = avaible - 1;
                            fs_file.seekp(links[i]);
                            fs_file.read(reinterpret_cast<char *>(&sublinks), sizeof(sublinks));
                            if(avaible) {
                                sublinks[0] = castDatablock(filesystem_data);
                                datablock_position = sublinks[0];
                                // aktualizace odkazu
                                fs_file.seekp(links[i]);
                                fs_file.write(reinterpret_cast<const char *>(&links), filesystem_data.super_block.cluster_size);
                            }
                        }
                        if(datablock_position || !avaible) break;
                    }
                }
            }
        }
    }

    // zapis inode
    fs_file.seekp(getINodePosition(filesystem_data, inode.nodeid));
    fs_file.write(reinterpret_cast<const char *>(&inode), sizeof(pseudo_inode));

    std::cout << " AVAIBLE DATABLOCKS: " << avaible_datablocks << "\n MAXIMUM DATABLOCKS PER INODE: " <<
    maximum_datablocks_per_inode << "\n USED DATABLOCKS BY INODE: " << used_datablocks_by_inode <<
    "\n AVAIBLE DATABLOCKS BY INODE: " << avaible_datablocks_by_inode << "\n AVAIBLE: " << avaible << std::endl;
    return datablock_position;
}