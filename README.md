# Semestrální práce z KIV/ZOS
Souborový systém založený na i-uzlech

## Adresáře
- doc - Dokumentace

## Požadavky
- cmake
- make
- gcc

## Překlad aplikace a první spuštění (Linux)

1) Přesun do rozbaleného adresáře: `cd KIV_ZOS_FS`

2) Vytvoření podadresáře _build_: `mkdir build` _(zde bude přeložený spustitelný soubor)_

3) Přesun do adresáře _build_: `cd KIV_ZOS_FS`

4) Překlad aplikace: `cmake .. && make`

5) Spuštění aplikace: `./PseudoFS filesystem.fs`

6) Formát souborového systému: `format 1000Mb` _(vytvoří soubor filesystem.fs v adresáři build o velikosti 1000 MB)_

## Omezení
- Jedná se jen o demonstraci fungování souborového systému založeného na i-uzlech.
- Maximální velikost souborového systému je 2 GB.
