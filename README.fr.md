# claet [<img src="https://github.com/vancolbert/trucsle/raw/main/flag-gb.svg" height="20" align="right"><img src="https://github.com/vancolbert/trucsle/raw/main/lang-en.svg" align="right">](README.md)
Copie du [code source du client](http://jeu.landes-eternelles.com/~ale/downloads.html)
pour le jeu de rôle en ligne [Landes Eternelles](http://www.landes-eternelles.com/).
J'ai fait quelques modifications mineures au but d'être compatible avec les compilateurs et bibliothèques récents.
### Les Branches
- **main**: Code source d'origine minimalement changé.
- **test**: Corrections plus compliquées et nouvelles fonctionnalités à évaluer.
- **cleanup**: Nettoyage agressif et inconciliable.
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
cp ./le.x86_64.linux.bin /répertoire/de/LandesEternelles/claet-le.x86_64.linux.bin
```
Selon les commands ci-dessus, il faut configurer `LD_LIBRARY_PATH` avant de
lancer l'exécutable:
```
cd /répertoire/de/LandesEternelles
export LD_LIBRARY_PATH="$p/lib"
./claet-le.x86_64.linux.bin
```
## Compilation croisée de Linux à Windows
En utilisant [setupmingw.py](https://raw.githubusercontent.com/vancolbert/trucsle/main/setupmingw.py)
et [claet.pkglist](https://raw.githubusercontent.com/vancolbert/trucsle/main/claet.pkglist)
on peut obtenir, configurer, et construire tous les
outils et bibliothèques nécessaires.
On devrait mettre en place `CFLAGS` et `CXXFLAGS` *avant* de lancer le script:
```
export CFLAGS='-O2 -g0'
export CXXFLAGS='-O2 -g0'
setupmingw.py -j`nproc` claet.pkglist
```
Une fois réussi, on peut faire:
```
cd /fichier/de/claet
/fichier/de/sys.i686-w64-mingw32/cross.env make -f Makefile.mingw
```
