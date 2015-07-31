# BTFS (bittorrent filesystem)

## What is this?

With BTFS, you can mount any **.torrent** file or **magnet link** and then use it as any read-only directory in your file tree. The contents of the files will be downloaded on-demand as they are read by applications. Tools like **ls**, **cat** and **cp** works as expected. Applications like **vlc** and **mplayer** can also work without changes.

## Example usage

    $ mkdir mnt
    $ btfs video.torrent mnt
    $ cd mnt
    $ vlc video.mp4

To unmount and shutdown:

    $ fusermount -u mnt

## Installing on a recent Ubuntu (Wily, Vivid or Trusty)

    $ sudo add-apt-repository ppa:johang/btfs
    $ sudo apt-get update
    $ sudo apt-get install btfs

## Dependencies

* fuse ("fuse" in Debian/Ubuntu)
* libtorrent ("libtorrent-rasterbar7" in Debian/Ubuntu)
* libcurl ("libcurl3" in Debian/Ubuntu)

## Building from git on a recent Ubuntu

    $ apt-get install autoconf automake libfuse-dev libtorrent-rasterbar-dev libcurl4-openssl-dev
    $ git clone https://github.com/johang/btfs.git btfs
    $ cd btfs
    $ autoreconf -i
    $ ./configure
    $ make
