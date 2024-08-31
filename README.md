# claet [<img src="https://github.com/vancolbert/trucsle/raw/main/flag-fr.svg" height="20" align="right"><img src="https://github.com/vancolbert/trucsle/raw/main/lang-fr.svg" align="right">](README.fr.md)
Github mirror of the game client [source code](http://jeu.landes-eternelles.com/~ale/downloads.html)
for the French online roleplaying game [Landes Eternelles](http://www.landes-eternelles.com/).
I have made minor changes for recent compilers and libraries.
## Branches
- **main**: Upstream source code with minimal changes.
- **test**: More involved fixes and new features needing evaluation.
- **cleanup**: Aggressive and incompatible sanitization.
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
## Cross compiling from Linux to Windows
Use [setupmingw.py](https://raw.githubusercontent.com/vancolbert/trucsle/main/setupmingw.py)
with [claet.pkglist](https://raw.githubusercontent.com/vancolbert/trucsle/main/claet.pkglist)
to download, configure, and build the required tools and libraries.
Set `CFLAGS` and `CXXFLAGS` *before* running the script:
```
export CFLAGS='-O2 -g0'
export CXXFLAGS='-O2 -g0'
setupmingw.py -j`nproc` claet.pkglist
```
If successful you can then do:
```
cd /path/to/claet
/path/to/sys.i686-w64-mingw32/cross.env make -f Makefile.mingw
```
