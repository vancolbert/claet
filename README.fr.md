# claet [<img src="https://github.com/vancolbert/trucsle/raw/main/flag-gb.svg" height="20" align="right"><img src="https://github.com/vancolbert/trucsle/raw/main/lang-en.svg" align="right">](README.md)

Le [code source](http://jeu.landes-eternelles.com/~ale/downloads.html) du client pour le jeu [Landes Eternelles](http://www.landes-eternelles.com/).

## Compilation sous Linux
```
make -f Makefile.linux
```
Pour plus d'informations venez voir au [forum](http://www.landes-eternelles.com/phpBB/viewforum.php?f=104).
Concernant Cal3D, il y a une version compatible disponible [ici](https://github.com/vancolbert/cal3d_12).
Par exemple:
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
