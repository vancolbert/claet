# claet [<img src="https://github.com/vancolbert/trucsle/raw/main/flag-fr.svg" height="20" align="right"><img src="https://github.com/vancolbert/trucsle/raw/main/lang-fr.svg" align="right">](README.fr.md)

Github mirror of the game client [source code](http://jeu.landes-eternelles.com/~ale/downloads.html)
for the French online roleplaying game [Landes Eternelles](http://www.landes-eternelles.com/).
I have made minor changes for recent compilers and libraries.

## Building on Linux
```
git clone https://github.com/vancolbert/claet
cd claet
make -f Makefile.linux
```
Most required libraries should be available from your distro.
If you have trouble with cal3d, try [this version](https://github.com/vancolbert/cal3d_12):
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
cp ./le.x86_64.linux.bin /path/to/LandesEternelles/claet-le.x86_64.linux.bin
```
Set `LD_LIBRARY_PATH` before running if you installed cal3d in a custom prefix:
```
cd /path/to/LandesEternelles
export LD_LIBRARY_PATH="$p/lib"
./claet-le.x86_64.linux.bin
```
