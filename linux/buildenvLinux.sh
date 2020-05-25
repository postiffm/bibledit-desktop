#!/bin/bash

if [ "$EUID" -ne 0 ]; then
  sudo "$0"
  exit
fi

echo "This script works on ubuntu Linux 18.04 LTS and on Debian Unstable and Arch Linux"

# I assume you have already done this:
# sudo apt install git
# cd ~
# git clone https://github.com/postiffm/bibledit-desktop.git

if apt --version 2> /dev/null; then
  apt install gitk
  apt install build-essential
  apt install autotools-dev
  apt install yelp yelp-tools #Dependencies for documentation compilation
  apt install libgtk-3-dev
  apt install rcs
  apt install sqlite3
  apt install libglibmm-2.4-dev libglibmm-2.4-1v5
  apt install libsqlite3-dev
  apt install libxml2-dev
  apt install libenchant-dev
  apt install libgtksourceview-3.0-1 libgtksourceview-3.0-dev
  # Below installs -dev and -doc as well
  apt install libwebkit2gtk-4.0-37 libwebkit2gtk-4.0-dev
# I'm not sure this package very heavy is still mandatory. BBD works perfectly without it.
#  apt install texlive-xetex
  apt install php7.4-cli
  apt install curl
  apt install intltool
  apt install libtool
  apt install autoconf-archive
  apt install gtk-3-examples
elif pacman --version 2> /dev/null; then
  # TODO: investigate if gtkhtml3 is necessary (it is in the AUR)
  pacman -S --needed --noconfirm base-devel gtk2 rcs sqlite \
    glibmm sqlite libxml2 enchant libgtksourceviewmm2 \
    texlive-core php curl intltool libtool autoconf-archive \
    webkitgtk2
fi

echo "You may also want to install development documentation if you are a developer."
echo "apt install devhelp"
echo "sudo add-apt-repository ppa:p12/ppa"
echo "sudo apt update"
echo "sudo apt install cppreference-doc-en-html"
echo "sudo apt install libxml2-doc"
echo "sudo apt install libgtk2.0-doc"
