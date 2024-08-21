# claet [<img src="https://github.com/vancolbert/trucsle/raw/main/flag-gb.svg" height="20" align="right"><img src="https://github.com/vancolbert/trucsle/raw/main/lang-en.svg" align="right">](README.md)

Copie du [code source du client](http://jeu.landes-eternelles.com/~ale/downloads.html)
pour le jeu de rôle en ligne [Landes Eternelles](http://www.landes-eternelles.com/).
J'ai fait quelques modifications mineures au but d'être compatible avec les compilateurs et bibliothèques récents. 

## Compilation sous Linux
```
git clone https://github.com/vancolbert/claet
cd claet
make -f Makefile.linux
```
Pour plus d'informations venez voir au [forum](http://www.landes-eternelles.com/phpBB/viewforum.php?f=104).
Concernant cal3d, il y a une version compatible disponible [ici](https://github.com/vancolbert/cal3d_12).
Par exemple:
```
git clone https://github.com/vancolbert/cal3d_12
cd cal3d_12
autoreconf -i
p=`realpath install-cal3d_12`
./configure --prefix="$p"
make install
cd ..
export PKG_CONFIG_PATH="$p/lib/pkgconfig"
make -f Makefile.linux EXTRA_INCLUDES=`pkg-config cal3d --cflags`
cp ./le.x86_64.linux.bin /fichier/de/LandesEternelles/claet-le.x86_64.linux.bin
```
Selon les commands ci-dessus, il faut configurer `LD_LIBRARY_PATH` avant de
lancer l'exécutable:
```
cd /fichier/de/LandesEternelles
export LD_LIBRARY_PATH="$p/lib"
./claet-le.x86_64.linux.bin
```
