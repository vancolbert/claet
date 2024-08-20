# claet [<img src="https://github.com/vancolbert/trucsle/raw/main/flag-fr.svg" height="20" align="right"><img src="https://github.com/vancolbert/trucsle/raw/main/lang-fr.svg" align="right">](README.fr.md)

[Landes Eternelles](http://www.landes-eternelles.com/) game client [source code](http://jeu.landes-eternelles.com/~ale/downloads.html).

## Compilation on Linux
```
make -f Makefile.linux
```
Most required libraries should be available from your distro.
If you have trouble with Cal3D, try [this version](https://github.com/vancolbert/cal3d_12).
For example:
```
$ git clone https://github.com/vancolbert/cal3d_12
$ cd cal3d_12
$ autoreconf -i
$ p=`realpath install`
$ ./configure --prefix=$p
$ make install
(cd claet)
$ export PKG_CONFIG_PATH=$p/lib/pkgconfig
$ make -f Makefile.linux EXTRA_INCLUDES=$(pkg-config cal3d --cflags)
```
