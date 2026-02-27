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

## Installing on Debian/Ubuntu

    # apt-get install btfs

## Installing on Arch Linux

    # pacman -S btfs

## Installing on Gentoo

    # emerge -av btfs
    
## Installing on Fedora

    # dnf install fuse-btfs
    
## Installing on Fedora OSTree

    $ rpm-ostree install fuse-btfs
    
## OpenSUSE

    # zypper install btfs
    
## Installing on macOS

Use [`brew`](https://brew.sh) to install on macOS.

    $ brew install btfs

## Dependencies (on Linux)

* fuse3 ("fuse3" in Ubuntu 22.04)
* libtorrent ("libtorrent-rasterbar8" in Ubuntu 22.04)
* libcurl ("libcurl4" in Ubuntu 22.04)

## Building from git on a recent Debian/Ubuntu

    $ sudo apt-get install autoconf automake libfuse3-dev libtorrent-rasterbar-dev libcurl4-openssl-dev g++
    $ git clone https://github.com/johang/btfs.git btfs
    $ cd btfs
    $ autoreconf -i
    $ ./configure
    $ make

And optionally, if you want to install it:

    $ make install

## Building on macOS

Use [`brew`](https://brew.sh) to get the dependencies.

    $ brew install --cask macfuse libtorrent-rasterbar autoconf automake pkg-config
    $ git clone https://github.com/johang/btfs.git btfs
    $ cd btfs
    $ autoreconf -i
    $ ./configure
    $ make

And optionally, if you want to install it:

    $ make install

## Building and running with Docker
Both `Dockerfile` and Docker Compose [manifest](./docker-compose.yml) are provided. They can be used both for deploying and for development (no need to install development toolchain in your machine, just Docker and Docker Compose).
You can use them to build and run from source a container which will mount [Sintel](https://cloud.blender.org/films/sintel) movie torrent in your `/tmp/btfs-docker/` dir by doing:
```
$ docker-compose up --build --force-recreate
```
Stop it by `CTRL+C`'ing or by executing `docker stop btfs` from another shell.

See the `docker-compose.yml` manifest file for more info and customizations.
