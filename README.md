# BTFS (bittorrent filesystem)

## Example usage

    $ mkdir mnt
    $ btfs video.torrent mnt
    $ cd mnt
    $ vlc video.mp4
    $ fusermount -u mnt

## Dependencies

* fuse ("fuse" in Debian/Ubuntu)
* libtorrent ("libtorrent-rasterbar7" in Debian/Ubuntu)

## Building from git on a recent Ubuntu

    $ apt-get install autoconf automake libfuse-dev libtorrent-rasterbar-dev
    $ git clone ... btfs
    $ cd btfs
    $ aclocal
    $ automake -a
    $ autoconf
    $ ./configure
    $ make
