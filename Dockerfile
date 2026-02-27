FROM gcc

RUN \
  apt-get update && \
  DEBIAN_FRONTEND=noninteractive \
    apt-get install -y \
      fuse \
      libfuse-dev \
      libtorrent-rasterbar-dev \
      libcurl4-openssl-dev \
  && \
  apt-get clean && \
  rm -rf /var/lib/apt/lists/*

COPY . /src/btfs/
WORKDIR /src/btfs/

RUN \
  autoreconf -fvi && \
  ./configure

RUN \
  make && \
  make install

ENTRYPOINT [ "btfs", "-f" ]
