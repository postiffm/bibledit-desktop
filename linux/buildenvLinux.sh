#!/bin/bash

if [ "$EUID" -ne 0 ]; then
  sudo "$0"
  exit
fi

echo "This script works on ubuntu Linux 17.04 LTS and Arch Linux"

# I assume you have already done this:
# sudo apt-get install git
# cd ~
# git clone https://github.com/postiffm/bibledit-desktop.git

if apt-get --version 2> /dev/null; then
  apt-get install gitk
  apt-get install build-essential
  apt-get install libgtk2.0-dev
  apt-get install yelp yelp-tools #Dependencies for documentation compilation
  #apt-get install libgtk-3-dev
  apt-get install rcs
  apt-get install sqlite3
  apt-get install libglibmm-2.4-dev libglibmm-2.4
  apt-get install libsqlite3-dev
  apt-get install libxml2-dev
  apt-get install libenchant-dev
  apt-get install libgtkhtml3.14-dev
  apt-get install libgtksourceview2.0-dev
  apt-get install libgtksourceview-3.0
  # Below installs -dev and -doc as well
  apt-get install libwebkit2gtk-4.0
  #apt-get install libwebkit-dev
  apt-get install libwebkitgtk-dev
  #apt-get install libwebkitgtk-3.0
  apt-get install texlive-xetex
  #apt-get install php5-cli <-- not available Ubuntu 17.04
  apt-get install php7.0-cli
  apt-get install curl
  apt-get install intltool
  apt-get install libtool
  apt-get install autoconf-archive
  apt-get install gtk-3-examples
elif pacman --version 2> /dev/null; then
  # TODO: investigate if gtkhtml3 is necessary (it is in the AUR)
  pacman -S --needed --noconfirm base-devel gtk2 rcs sqlite \
    glibmm sqlite libxml2 enchant libgtksourceviewmm2 \
    texlive-core php curl intltool libtool autoconf-archive \
    webkitgtk2
fi

echo "You may also want to install development documentation if you are a developer."
echo "apt-get install devhelp"
echo "sudo add-apt-repository ppa:p12/ppa"
echo "sudo apt-get update"
echo "sudo apt-get install cppreference-doc-en-html"
echo "sudo apt-get install libxml2-doc"
echo "sudo apt-get install libgtk2.0-doc"

# This is an error that threw me for some reason for a while
# % ./configure
# checking for WEBKIT... no
# configure: error: libwebkit development version >= 1.0.0 is needed.
# What we need is this:
#sudo apt-get install libwebkitgtk-dev
