#!/bin/bash

apt-get install -y wget
wget https://boostorg.jfrog.io/artifactory/main/release/1.66.0/source/boost_1_66_0.tar.bz2
apt-get install -y tar
apt-get install -y bzip2
tar --bzip2 -xf boost_1_66_0.tar.bz2
rm boost_1_66_0.tar.bz2
cd boost_1_66_0 && ./bootstrap.sh --prefix=/usr/local && ./b2 install
