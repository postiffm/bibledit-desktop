#!/bin/bash
# Copy all necessary files from git repo to Windows to make a "nice" installation.
# Run this from the bibledit/gtk build directory like this:
# windows/installWin.sh [options]
# Also can create a self-extracting installer (see -g option) to distribute
# the package to other computers.

# To change the version number, change the following line and
# the version in ../configure.ac, as well as any shortcuts.
VERSION="4.16"

full=1
docs=0
quick=0
do_strip=0
makeinstall=0
function usage
{
	echo "Usage: installWin.sh [options]"
	echo ""
	echo "Copy files to C:\Program Files to make a full"
	echo "running environment for Bibledit. Options include:"
	echo ""
	echo "       --docs | -d   Install just docs and quit immediately."
	echo "       --full | -f   Install everything (default behavior)."
	echo "       --help | -h   Print usage information and exit."
	echo "       --quick | -q  Only install bibledit binaries; not all libraries "
	echo "                     or other support files."
	echo "       --strip | -s  Shrink binary size for faster copy/download."
	echo "       --generateinstall | -g  Generate installer executable. Requires --full to also be specified."
	echo ""
	echo "NOTE: We do not support installing a 32-bit version of Bibledit-Desktop on a 64-bit"
	echo "      computer [in Program Files (x86)]. It is possible to do, but it is buggy."
	
}

# Process command-line arguments
while [ "$1" != "" ]; do
    case $1 in
		-s | --strip )			do_strip=1
								;;
        -q | --quick )          quick=1
                                ;;
        -d | --docs )           docs=1
                                ;;
        -f | --full )           full=1
                                ;;
        -g | --generateinstall )makeinstall=1
                                ;;

        -h | --help )           usage
                                exit
                                ;;
        * )                     usage
                                exit 1
    esac
    shift
done

#------------------------------------------------------------------------------------------
# Figure out if we are in a 32-bit or 64-bit environment and target Windows accordingly
#------------------------------------------------------------------------------------------
if [ -n "$MSYSTEM" ]
then
  case "$MSYSTEM" in
    MINGW32)
      # Program Files for 32-bit programs
	  PROGRAMFILES="Program Files"
	  # Source directory for binaries and such
	  MINGWDIR="mingw32"
	  WINBITS="win32"
    ;;
    MINGW64)
	  # Program Files for 64-bit programs
      PROGRAMFILES="Program Files"
	  # Source directory for binaries and such
	  MINGWDIR="mingw64"
	  WINBITS="win64"
    ;;
    MSYS)
      echo "I am not set up to install from an msys shell. Please use"
	  echo "a MINGW32 or MINGW64 shell."
	  exit 1
    ;;
    *)
      echo "Unknown value $MSYSTEM in MSYSTEM environment variable, so I don't know what to do."
      exit 2
    ;;
  esac
else
  echo "Can't find MSYSTEM environment variable, so I don't know what to do."
  exit 3
fi

# Because of the space in Program Files, I have to quote this carefully
# when I use it.
BIN="/c/$PROGRAMFILES/Bibledit-$VERSION/editor/bin"
SHARE="/c/$PROGRAMFILES/Bibledit-$VERSION/editor/share"
LIB="/c/$PROGRAMFILES/Bibledit-$VERSION/editor/lib"
ETC="/c/$PROGRAMFILES/Bibledit-$VERSION/editor/etc"
DEST_TMP="/c/$PROGRAMFILES/Bibledit-$VERSION/tmp"
DEST_USRBIN="/c/$PROGRAMFILES/Bibledit-$VERSION/usr/bin"
# Take note: this is the 64-bit version of stuff
DLLS="/$MINGWDIR/bin"
THEMES="/$MINGWDIR/share/themes"
ENGINES="/$MINGWDIR/lib/gtk-2.0"
MINGWBIN="/$MINGWDIR/bin"
USRBIN="/usr/bin"

# If we are in --quick mode, then quit now, don't do more work
if [ "$docs" = "1" ]; then
    cp -R doc/site "$SHARE/bibledit"
    exit 0;
fi

# I find I sometimes need time to kill a previous instance of the program or
# something else, and this warning wakes me up to that need.
echo "I assume you are running this from your-home/bibledit-desktop."
echo "I also assume this shell is running in elevated/administrator mode!"
echo "If not, hit Ctrl-C now and correct those problems!"
sleep 3

echo "Copying Bibledit executables to $BIN..."
mkdir -v -p "$BIN"
cp ./src/.libs/bibledit-desktop.exe "$BIN"
cp ./src/.libs/bibledit-rdwrt.exe "$BIN"
#cp ./src/.libs/concordance.exe "$BIN"
cp ./git/.libs/bibledit-git.exe "$BIN"
cp ./shutdown/.libs/bibledit-shutdown.exe "$BIN"
cp -R ./windows/bibledit.ico "$BIN"

# Added 3/7/2017
echo "Copying language / i18n packages to $SHARE/locale and scripts..."
mkdir -v -p "$SHARE/locale/fr/LC_MESSAGES"
cp ./po/fr.gmo "$SHARE/locale/fr/LC_MESSAGES/bibledit-desktop.mo"
cp ./windows/bibledit-fr.cmd "$BIN"
# Above added 3/7/2017

# If we are in --strip mode, then shrink the executables in the dest directory
# I opt to leave them as-is in the source directory in case debug info is needed
if [ "$do_strip" = "1" ]; then
    strip "$BIN/bibledit-desktop.exe"
	strip "$BIN/bibledit-rdwrt.exe"
	strip "$BIN/concordance.exe"
	strip "$BIN/bibledit-git.exe"
	strip "$BIN/bibledit-shutdown.exe"
fi


# If we are in --quick mode, then quit now, don't do more work
if [ "$quick" = "1" ]; then
    exit 0;
fi

#which: no egrep.exe in (/mingw64/bin:/usr/local/bin:/usr/bin:/bin:/c/Program Files (x86)/Intel/iCLS Client:/c/Program Files/Intel/iCLS Client:/c/Windows/system32:/c/Windows:/c/Windows/System32/Wbem:/c/Windows/System32/WindowsPowerShell/v1.0:/c/Program Files/Intel/Intel(R) Management Engine Components/DAL:/c/Program Files (x86)/Intel/Intel(R) Management Engine Components/DAL:/c/Program Files/Intel/Intel(R) Management Engine Components/IPT:/c/Program Files (x86)/Intel/Intel(R) Management Engine Components/IPT:/c/Program Files (x86)/ATI Technologies/ATI.ACE/Core-Static:/c/Program Files/Lame3.99.5-64:/c/Program Files (x86)/GNU/GnuPG/pub:/c/Strawberry/c/bin:/c/Strawberry/perl/site/bin:/c/Strawberry/perl/bin:/c/Program Files (x86)/Windows Kits/8.1/Windows Performance Toolkit:/c/Program Files (x86)/AMD/ATI.ACE/Core-Static:/c/Program Files/Intel/WiFi/bin:/c/Program Files/Common Files/Intel/WirelessCommon:/c/Program Files (x86)/Skype/Phone:/usr/bin/site_perl:/usr/bin/vendor_perl:/usr/bin/core_perl)
#which: no fgrep.exe in (/mingw64/bin:/usr/local/bin:/usr/bin:/bin:/c/Program Files (x86)/Intel/iCLS Client:/c/Program Files/Intel/iCLS Client:/c/Windows/system32:/c/Windows:/c/Windows/System32/Wbem:/c/Windows/System32/WindowsPowerShell/v1.0:/c/Program Files/Intel/Intel(R) Management Engine Components/DAL:/c/Program Files (x86)/Intel/Intel(R) Management Engine Components/DAL:/c/Program Files/Intel/Intel(R) Management Engine Components/IPT:/c/Program Files (x86)/Intel/Intel(R) Management Engine Components/IPT:/c/Program Files (x86)/ATI Technologies/ATI.ACE/Core-Static:/c/Program Files/Lame3.99.5-64:/c/Program Files (x86)/GNU/GnuPG/pub:/c/Strawberry/c/bin:/c/Strawberry/perl/site/bin:/c/Strawberry/perl/bin:/c/Program Files (x86)/Windows Kits/8.1/Windows Performance Toolkit:/c/Program Files (x86)/AMD/ATI.ACE/Core-Static:/c/Program Files/Intel/WiFi/bin:/c/Program Files/Common Files/Intel/WirelessCommon:/c/Program Files (x86)/Skype/Phone:/usr/bin/site_perl:/usr/bin/vendor_perl:/usr/bin/core_perl)
#which: no funzip.exe in (/mingw64/bin:/usr/local/bin:/usr/bin:/bin:/c/Program Files (x86)/Intel/iCLS Client:/c/Program Files/Intel/iCLS Client:/c/Windows/system32:/c/Windows:/c/Windows/System32/Wbem:/c/Windows/System32/WindowsPowerShell/v1.0:/c/Program Files/Intel/Intel(R) Management Engine Components/DAL:/c/Program Files (x86)/Intel/Intel(R) Management Engine Components/DAL:/c/Program Files/Intel/Intel(R) Management Engine Components/IPT:/c/Program Files (x86)/Intel/Intel(R) Management Engine Components/IPT:/c/Program Files (x86)/ATI Technologies/ATI.ACE/Core-Static:/c/Program Files/Lame3.99.5-64:/c/Program Files (x86)/GNU/GnuPG/pub:/c/Strawberry/c/bin:/c/Strawberry/perl/site/bin:/c/Strawberry/perl/bin:/c/Program Files (x86)/Windows Kits/8.1/Windows Performance Toolkit:/c/Program Files (x86)/AMD/ATI.ACE/Core-Static:/c/Program Files/Intel/WiFi/bin:/c/Program Files/Common Files/Intel/WirelessCommon:/c/Program Files (x86)/Skype/Phone:/usr/bin/site_perl:/usr/bin/vendor_perl:/usr/bin/core_perl)
#which: no pgawk.exe in (/mingw64/bin:/usr/local/bin:/usr/bin:/bin:/c/Program Files (x86)/Intel/iCLS Client:/c/Program Files/Intel/iCLS Client:/c/Windows/system32:/c/Windows:/c/Windows/System32/Wbem:/c/Windows/System32/WindowsPowerShell/v1.0:/c/Program Files/Intel/Intel(R) Management Engine Components/DAL:/c/Program Files (x86)/Intel/Intel(R) Management Engine Components/DAL:/c/Program Files/Intel/Intel(R) Management Engine Components/IPT:/c/Program Files (x86)/Intel/Intel(R) Management Engine Components/IPT:/c/Program Files (x86)/ATI Technologies/ATI.ACE/Core-Static:/c/Program Files/Lame3.99.5-64:/c/Program Files (x86)/GNU/GnuPG/pub:/c/Strawberry/c/bin:/c/Strawberry/perl/site/bin:/c/Strawberry/perl/bin:/c/Program Files (x86)/Windows Kits/8.1/Windows Performance Toolkit:/c/Program Files (x86)/AMD/ATI.ACE/Core-Static:/c/Program Files/Intel/WiFi/bin:/c/Program Files/Common Files/Intel/WirelessCommon:/c/Program Files (x86)/Skype/Phone:/usr/bin/site_perl:/usr/bin/vendor_perl:/usr/bin/core_perl)
#which: no pgawk-3.1.7.exe in (/mingw64/bin:/usr/local/bin:/usr/bin:/bin:/c/Program Files (x86)/Intel/iCLS Client:/c/Program Files/Intel/iCLS Client:/c/Windows/system32:/c/Windows:/c/Windows/System32/Wbem:/c/Windows/System32/WindowsPowerShell/v1.0:/c/Program Files/Intel/Intel(R) Management Engine Components/DAL:/c/Program Files (x86)/Intel/Intel(R) Management Engine Components/DAL:/c/Program Files/Intel/Intel(R) Management Engine Components/IPT:/c/Program Files (x86)/Intel/Intel(R) Management Engine Components/IPT:/c/Program Files (x86)/ATI Technologies/ATI.ACE/Core-Static:/c/Program Files/Lame3.99.5-64:/c/Program Files (x86)/GNU/GnuPG/pub:/c/Strawberry/c/bin:/c/Strawberry/perl/site/bin:/c/Strawberry/perl/bin:/c/Program Files (x86)/Windows Kits/8.1/Windows Performance Toolkit:/c/Program Files (x86)/AMD/ATI.ACE/Core-Static:/c/Program Files/Intel/WiFi/bin:/c/Program Files/Common Files/Intel/WirelessCommon:/c/Program Files (x86)/Skype/Phone:/usr/bin/site_perl:/usr/bin/vendor_perl:/usr/bin/core_perl)
#which: no rcsclean.exe in (/mingw64/bin:/usr/local/bin:/usr/bin:/bin:/c/Program Files (x86)/Intel/iCLS Client:/c/Program Files/Intel/iCLS Client:/c/Windows/system32:/c/Windows:/c/Windows/System32/Wbem:/c/Windows/System32/WindowsPowerShell/v1.0:/c/Program Files/Intel/Intel(R) Management Engine Components/DAL:/c/Program Files (x86)/Intel/Intel(R) Management Engine Components/DAL:/c/Program Files/Intel/Intel(R) Management Engine Components/IPT:/c/Program Files (x86)/Intel/Intel(R) Management Engine Components/IPT:/c/Program Files (x86)/ATI Technologies/ATI.ACE/Core-Static:/c/Program Files/Lame3.99.5-64:/c/Program Files (x86)/GNU/GnuPG/pub:/c/Strawberry/c/bin:/c/Strawberry/perl/site/bin:/c/Strawberry/perl/bin:/c/Program Files (x86)/Windows Kits/8.1/Windows Performance Toolkit:/c/Program Files (x86)/AMD/ATI.ACE/Core-Static:/c/Program Files/Intel/WiFi/bin:/c/Program Files/Common Files/Intel/WirelessCommon:/c/Program Files (x86)/Skype/Phone:/usr/bin/site_perl:/usr/bin/vendor_perl:/usr/bin/core_perl)
#which: no rcsdiff.exe in (/mingw64/bin:/usr/local/bin:/usr/bin:/bin:/c/Program Files (x86)/Intel/iCLS Client:/c/Program Files/Intel/iCLS Client:/c/Windows/system32:/c/Windows:/c/Windows/System32/Wbem:/c/Windows/System32/WindowsPowerShell/v1.0:/c/Program Files/Intel/Intel(R) Management Engine Components/DAL:/c/Program Files (x86)/Intel/Intel(R) Management Engine Components/DAL:/c/Program Files/Intel/Intel(R) Management Engine Components/IPT:/c/Program Files (x86)/Intel/Intel(R) Management Engine Components/IPT:/c/Program Files (x86)/ATI Technologies/ATI.ACE/Core-Static:/c/Program Files/Lame3.99.5-64:/c/Program Files (x86)/GNU/GnuPG/pub:/c/Strawberry/c/bin:/c/Strawberry/perl/site/bin:/c/Strawberry/perl/bin:/c/Program Files (x86)/Windows Kits/8.1/Windows Performance Toolkit:/c/Program Files (x86)/AMD/ATI.ACE/Core-Static:/c/Program Files/Intel/WiFi/bin:/c/Program Files/Common Files/Intel/WirelessCommon:/c/Program Files (x86)/Skype/Phone:/usr/bin/site_perl:/usr/bin/vendor_perl:/usr/bin/core_perl)
#which: no rcsmerge.exe in (/mingw64/bin:/usr/local/bin:/usr/bin:/bin:/c/Program Files (x86)/Intel/iCLS Client:/c/Program Files/Intel/iCLS Client:/c/Windows/system32:/c/Windows:/c/Windows/System32/Wbem:/c/Windows/System32/WindowsPowerShell/v1.0:/c/Program Files/Intel/Intel(R) Management Engine Components/DAL:/c/Program Files (x86)/Intel/Intel(R) Management Engine Components/DAL:/c/Program Files/Intel/Intel(R) Management Engine Components/IPT:/c/Program Files (x86)/Intel/Intel(R) Management Engine Components/IPT:/c/Program Files (x86)/ATI Technologies/ATI.ACE/Core-Static:/c/Program Files/Lame3.99.5-64:/c/Program Files (x86)/GNU/GnuPG/pub:/c/Strawberry/c/bin:/c/Strawberry/perl/site/bin:/c/Strawberry/perl/bin:/c/Program Files (x86)/Windows Kits/8.1/Windows Performance Toolkit:/c/Program Files (x86)/AMD/ATI.ACE/Core-Static:/c/Program Files/Intel/WiFi/bin:/c/Program Files/Common Files/Intel/WirelessCommon:/c/Program Files (x86)/Skype/Phone:/usr/bin/site_perl:/usr/bin/vendor_perl:/usr/bin/core_perl)
#which: no unzipsfx.exe in (/mingw64/bin:/usr/local/bin:/usr/bin:/bin:/c/Program Files (x86)/Intel/iCLS Client:/c/Program Files/Intel/iCLS Client:/c/Windows/system32:/c/Windows:/c/Windows/System32/Wbem:/c/Windows/System32/WindowsPowerShell/v1.0:/c/Program Files/Intel/Intel(R) Management Engine Components/DAL:/c/Program Files (x86)/Intel/Intel(R) Management Engine Components/DAL:/c/Program Files/Intel/Intel(R) Management Engine Components/IPT:/c/Program Files (x86)/Intel/Intel(R) Management Engine Components/IPT:/c/Program Files (x86)/ATI Technologies/ATI.ACE/Core-Static:/c/Program Files/Lame3.99.5-64:/c/Program Files (x86)/GNU/GnuPG/pub:/c/Strawberry/c/bin:/c/Strawberry/perl/site/bin:/c/Strawberry/perl/bin:/c/Program Files (x86)/Windows Kits/8.1/Windows Performance Toolkit:/c/Program Files (x86)/AMD/ATI.ACE/Core-Static:/c/Program Files/Intel/WiFi/bin:/c/Program Files/Common Files/Intel/WirelessCommon:/c/Program Files (x86)/Skype/Phone:/usr/bin/site_perl:/usr/bin/vendor_perl:/usr/bin/core_perl)
#which: no zipcloak.exe in (/mingw64/bin:/usr/local/bin:/usr/bin:/bin:/c/Program Files (x86)/Intel/iCLS Client:/c/Program Files/Intel/iCLS Client:/c/Windows/system32:/c/Windows:/c/Windows/System32/Wbem:/c/Windows/System32/WindowsPowerShell/v1.0:/c/Program Files/Intel/Intel(R) Management Engine Components/DAL:/c/Program Files (x86)/Intel/Intel(R) Management Engine Components/DAL:/c/Program Files/Intel/Intel(R) Management Engine Components/IPT:/c/Program Files (x86)/Intel/Intel(R) Management Engine Components/IPT:/c/Program Files (x86)/ATI Technologies/ATI.ACE/Core-Static:/c/Program Files/Lame3.99.5-64:/c/Program Files (x86)/GNU/GnuPG/pub:/c/Strawberry/c/bin:/c/Strawberry/perl/site/bin:/c/Strawberry/perl/bin:/c/Program Files (x86)/Windows Kits/8.1/Windows Performance Toolkit:/c/Program Files (x86)/AMD/ATI.ACE/Core-Static:/c/Program Files/Intel/WiFi/bin:/c/Program Files/Common Files/Intel/WirelessCommon:/c/Program Files (x86)/Skype/Phone:/usr/bin/site_perl:/usr/bin/vendor_perl:/usr/bin/core_perl)
#which: no zipinfo.exe in (/mingw64/bin:/usr/local/bin:/usr/bin:/bin:/c/Program Files (x86)/Intel/iCLS Client:/c/Program Files/Intel/iCLS Client:/c/Windows/system32:/c/Windows:/c/Windows/System32/Wbem:/c/Windows/System32/WindowsPowerShell/v1.0:/c/Program Files/Intel/Intel(R) Management Engine Components/DAL:/c/Program Files (x86)/Intel/Intel(R) Management Engine Components/DAL:/c/Program Files/Intel/Intel(R) Management Engine Components/IPT:/c/Program Files (x86)/Intel/Intel(R) Management Engine Components/IPT:/c/Program Files (x86)/ATI Technologies/ATI.ACE/Core-Static:/c/Program Files/Lame3.99.5-64:/c/Program Files (x86)/GNU/GnuPG/pub:/c/Strawberry/c/bin:/c/Strawberry/perl/site/bin:/c/Strawberry/perl/bin:/c/Program Files (x86)/Windows Kits/8.1/Windows Performance Toolkit:/c/Program Files (x86)/AMD/ATI.ACE/Core-Static:/c/Program Files/Intel/WiFi/bin:/c/Program Files/Common Files/Intel/WirelessCommon:/c/Program Files (x86)/Skype/Phone:/usr/bin/site_perl:/usr/bin/vendor_perl:/usr/bin/core_perl)
#which: no zipnote.exe in (/mingw64/bin:/usr/local/bin:/usr/bin:/bin:/c/Program Files (x86)/Intel/iCLS Client:/c/Program Files/Intel/iCLS Client:/c/Windows/system32:/c/Windows:/c/Windows/System32/Wbem:/c/Windows/System32/WindowsPowerShell/v1.0:/c/Program Files/Intel/Intel(R) Management Engine Components/DAL:/c/Program Files (x86)/Intel/Intel(R) Management Engine Components/DAL:/c/Program Files/Intel/Intel(R) Management Engine Components/IPT:/c/Program Files (x86)/Intel/Intel(R) Management Engine Components/IPT:/c/Program Files (x86)/ATI Technologies/ATI.ACE/Core-Static:/c/Program Files/Lame3.99.5-64:/c/Program Files (x86)/GNU/GnuPG/pub:/c/Strawberry/c/bin:/c/Strawberry/perl/site/bin:/c/Strawberry/perl/bin:/c/Program Files (x86)/Windows Kits/8.1/Windows Performance Toolkit:/c/Program Files (x86)/AMD/ATI.ACE/Core-Static:/c/Program Files/Intel/WiFi/bin:/c/Program Files/Common Files/Intel/WirelessCommon:/c/Program Files (x86)/Skype/Phone:/usr/bin/site_perl:/usr/bin/vendor_perl:/usr/bin/core_perl)
#which: no zipsplit.exe in (/mingw64/bin:/usr/local/bin:/usr/bin:/bin:/c/Program Files (x86)/Intel/iCLS Client:/c/Program Files/Intel/iCLS Client:/c/Windows/system32:/c/Windows:/c/Windows/System32/Wbem:/c/Windows/System32/WindowsPowerShell/v1.0:/c/Program Files/Intel/Intel(R) Management Engine Components/DAL:/c/Program Files (x86)/Intel/Intel(R) Management Engine Components/DAL:/c/Program Files/Intel/Intel(R) Management Engine Components/IPT:/c/Program Files (x86)/Intel/Intel(R) Management Engine Components/IPT:/c/Program Files (x86)/ATI Technologies/ATI.ACE/Core-Static:/c/Program Files/Lame3.99.5-64:/c/Program Files (x86)/GNU/GnuPG/pub:/c/Strawberry/c/bin:/c/Strawberry/perl/site/bin:/c/Strawberry/perl/bin:/c/Program Files (x86)/Windows Kits/8.1/Windows Performance Toolkit:/c/Program Files (x86)/AMD/ATI.ACE/Core-Static:/c/Program Files/Intel/WiFi/bin:/c/Program Files/Common Files/Intel/WirelessCommon:/c/Program Files (x86)/Skype/Phone:/usr/bin/site_perl:/usr/bin/vendor_perl:/usr/bin/core_perl)

echo "Copying libgcc from $DLLS to $BIN"
if [ -e "$DLLS/libgcc_s_seh-1.dll" ]
then
  echo "  64-bit: Copying libgcc_s_seh-1"
  cp $DLLS/libgcc_s_seh-1.dll "$BIN"
fi
if [ -e "$DLLS/libgcc_s_dw2-1.dll" ]
then
  # Is this called msys-gcc_s-1.dll in future editions of msys2?
  echo "  32-bit: Copying libgcc_s_dw2-1"
  cp $DLLS/libgcc_s_dw2-1.dll "$BIN"
fi

echo "Copying DLLs from $DLLS to $BIN..."
cp $DLLS/libcairo-2.dll "$BIN"
cp $DLLS/libenchant.dll "$BIN"
# I think I also need, but not sure
#libenchant_myspell.dll libenchant_ispell.dll
# which reside in /$MINGWDIR/lib/enchant/...
cp $DLLS/libgdk-win32-2.0-0.dll "$BIN"
# Below added 6/24/2016 in support of gtk3 transition
cp $DLLS/libgdk-3-0.dll "$BIN"
cp $DLLS/libcairo-gobject-2.dll "$BIN"
cp $DLLS/libepoxy-0.dll "$BIN"
cp $DLLS/libgtk-3-0.dll "$BIN"
cp $DLLS/libgtksourceview-3.0-1.dll "$BIN"
cp $DLLS/libwebkitgtk-3.0-0.dll "$BIN"
cp $DLLS/libjavascriptcoregtk-3.0-0.dll "$BIN"
# Above added 6/24/20116
cp $DLLS/libglib-2.0-0.dll "$BIN"
cp $DLLS/libgobject-2.0-0.dll "$BIN"
cp $DLLS/libgtk-win32-2.0-0.dll "$BIN"
cp $DLLS/libgtksourceview-2.0-0.dll "$BIN"
cp $DLLS/libintl-8.dll "$BIN"
cp $DLLS/libpango-1.0-0.dll "$BIN" 
cp $DLLS/libpangocairo-1.0-0.dll "$BIN"
cp $DLLS/libsoup-2.4-1.dll "$BIN"
cp $DLLS/libsqlite3-0.dll "$BIN"
# Next two added 4/15/2016 after changing over to glibmm/ustring
cp $DLLS/libglibmm-2.4-1.dll "$BIN"
cp $DLLS/libsigc-2.0-0.dll "$BIN"
cp $DLLS/libwebkitgtk-1.0-0.dll "$BIN"
cp $DLLS/libxml2-2.dll "$BIN"
cp $DLLS/libatk-1.0-0.dll "$BIN"
cp $DLLS/libffi-6.dll "$BIN"
cp $DLLS/libfontconfig-1.dll "$BIN"
cp $DLLS/libfreetype-6.dll "$BIN"
cp $DLLS/libgdk_pixbuf-2.0-0.dll "$BIN"
cp $DLLS/libgio-2.0-0.dll "$BIN"
cp $DLLS/libgmodule-2.0-0.dll "$BIN"
cp $DLLS/libgstapp-1.0-0.dll "$BIN"
cp $DLLS/libgstaudio-1.0-0.dll "$BIN"
cp $DLLS/libgstbase-1.0-0.dll "$BIN"
cp $DLLS/libgstfft-1.0-0.dll "$BIN"
cp $DLLS/libgstpbutils-1.0-0.dll "$BIN"
cp $DLLS/libgstreamer-1.0-0.dll "$BIN"
cp $DLLS/libgstvideo-1.0-0.dll "$BIN"
cp $DLLS/libharfbuzz-0.dll "$BIN"
cp $DLLS/libharfbuzz-icu-0.dll "$BIN"
cp $DLLS/libiconv-2.dll "$BIN"
# The next file is a must have
cp $DLLS/libicuin58.dll "$BIN"
cp $DLLS/libicuuc58.dll "$BIN"
cp $DLLS/libicudt58.dll "$BIN"
cp $DLLS/libicutu58.dll "$BIN"
cp $DLLS/libicutest58.dll "$BIN"
#cp $DLLS/libiculx58.dll "$BIN"
#cp $DLLS/libicule58.dll "$BIN"
cp $DLLS/libicuio58.dll "$BIN"
cp $DLLS/libjavascriptcoregtk-1.0-0.dll "$BIN"
cp $DLLS/libjpeg-8.dll "$BIN"
cp $DLLS/libbz2-1.dll "$BIN"
cp $DLLS/libexpat-1.dll "$BIN"
cp $DLLS/libgeoclue-0.dll "$BIN"
cp $DLLS/libgsttag-1.0-0.dll "$BIN"
cp $DLLS/liblzma-5.dll "$BIN"
cp $DLLS/liborc-0.4-0.dll "$BIN"
cp $DLLS/libpangoft2-1.0-0.dll "$BIN"
cp $DLLS/libpangowin32-1.0-0.dll "$BIN"
cp $DLLS/libpixman-1-0.dll "$BIN"
cp $DLLS/libpng16-16.dll "$BIN"
cp $DLLS/libwebp-7.dll "$BIN"
cp $DLLS/libxslt-1.dll "$BIN"
cp $DLLS/libdbus-glib-1-2.dll "$BIN"
cp $DLLS/libdbus-1-3.dll "$BIN"
cp $DLLS/libstdc++-6.dll "$BIN"
cp $DLLS/zlib1.dll "$BIN"
cp $DLLS/libwinpthread-1.dll "$BIN"
# Below added 3/24/2016. Very important to have one or more of these
cp $DLLS/libexslt-0.dll "$BIN"
cp $DLLS/libgailutil-18.dll "$BIN"
cp $DLLS/libgailutil-3-0.dll "$BIN"
cp $DLLS/libgmp-10.dll "$BIN"
cp $DLLS/libgmpxx-4.dll "$BIN"
cp $DLLS/libgnutls-30.dll "$BIN"
cp $DLLS/libgnutlsxx-28.dll "$BIN"
# Below was /mingw64/bin/libnettle-6-1.dll
cp $DLLS/libnettle-6.dll "$BIN"
cp $DLLS/libtiff-5.dll "$BIN"
cp $DLLS/libtiffxx-5.dll "$BIN"
cp $DLLS/libp11-kit-0.dll "$BIN"
cp $DLLS/libtasn1-6.dll "$BIN"
# Above added 3/24/2016.
# Below added 3/25/2016
# Below was libhogweed-4-1.dll
cp $DLLS/libhogweed-4.dll "$BIN"
cp $DLLS/libjasper-4.dll "$BIN"
cp $DLLS/libgthread-2.0-0.dll "$BIN"
cp $DLLS/libhunspell-1.6-0.dll "$BIN"
cp $DLLS/libpng-config "$BIN"
# The next file is a must have
cp $DLLS/libpng16-16.dll "$BIN"
cp $DLLS/libpng16-config "$BIN"
cp $DLLS/icu-config "$BIN"
# Above added 3/25/2016
# Below added 6/9/2016
cp $DLLS/libpcre-1.dll "$BIN"
cp $DLLS/libgraphite2.dll "$BIN"
# Above added 6/9/2016

echo "Copying various executables to $BIN"
cp $USRBIN/bison.exe "$BIN"
cp $USRBIN/cat.exe "$BIN"
cp $USRBIN/cmp.exe "$BIN"
cp $USRBIN/cp.exe "$BIN"
cp $USRBIN/curl.exe "$BIN"
cp $USRBIN/date.exe "$BIN"
cp $USRBIN/diff.exe "$BIN"
cp $USRBIN/diff3.exe "$BIN"
cp $USRBIN/echo.exe "$BIN"
# Doesn't exist in msys2
#cp $USRBIN/egrep.exe "$BIN"
cp $USRBIN/false.exe "$BIN"
# Doesn't exist in msys2
#cp $USRBIN/fgrep.exe "$BIN"
cp $USRBIN/find.exe "$BIN"
cp $USRBIN/flex.exe "$BIN"
cp $USRBIN/gawk.exe "$BIN"
cp $USRBIN/gperf.exe "$BIN"
cp $USRBIN/grep.exe "$BIN"
cp $USRBIN/gzip.exe "$BIN"
cp $USRBIN/iconv.exe "$BIN"
cp $USRBIN/info.exe "$BIN"
cp $USRBIN/join.exe "$BIN"
cp $USRBIN/less.exe "$BIN"
cp $USRBIN/lessecho.exe "$BIN"
cp $USRBIN/lesskey.exe "$BIN"
cp $USRBIN/ln.exe "$BIN"
cp $USRBIN/ls.exe "$BIN"
cp $USRBIN/md5sum.exe "$BIN"
cp $USRBIN/merge.exe "$BIN"
cp $USRBIN/mkdir.exe "$BIN"
cp $USRBIN/mv.exe "$BIN"
cp $USRBIN/paste.exe "$BIN"
cp $USRBIN/patch.exe "$BIN"
cp $USRBIN/printf.exe "$BIN"
cp $USRBIN/ps.exe "$BIN"
cp $USRBIN/pwd.exe "$BIN"
cp $USRBIN/rcs.exe "$BIN"
cp $USRBIN/rm.exe "$BIN"
cp $USRBIN/rmdir.exe "$BIN"
cp $USRBIN/sdiff.exe "$BIN"
cp $USRBIN/sed.exe "$BIN"
cp $USRBIN/sleep.exe "$BIN"
cp $USRBIN/sort.exe "$BIN"
cp $USRBIN/split.exe "$BIN"
cp $USRBIN/tail.exe "$BIN"
cp $USRBIN/tar.exe "$BIN"
cp $USRBIN/tee.exe "$BIN"
cp $USRBIN/touch.exe "$BIN"
cp $USRBIN/tr.exe "$BIN"
cp $USRBIN/true.exe "$BIN"
cp $USRBIN/uname.exe "$BIN"
cp $USRBIN/wc.exe "$BIN"
cp $USRBIN/wget.exe "$BIN"
cp $USRBIN/xargs.exe "$BIN"
cp $USRBIN/xml2ag.exe "$BIN"
cp $USRBIN/msys-intl-8.dll "$BIN"
cp $USRBIN/msys-iconv-2.dll "$BIN"
cp $USRBIN/msys-2.0.dll "$BIN"
cp $MINGWBIN/nm.exe "$BIN"
cp $MINGWBIN/bunzip2.exe "$BIN"
cp $MINGWBIN/bzip2.exe "$BIN"
cp $MINGWBIN/size.exe "$BIN"
cp $MINGWBIN/sqlite3.exe "$BIN"
cp $MINGWBIN/strings.exe "$BIN"
cp $MINGWBIN/strip.exe "$BIN"
cp $MINGWBIN/unxz.exe "$BIN"
cp $MINGWBIN/xz.exe "$BIN"
cp $MINGWBIN/xzcat.exe "$BIN"
cp $MINGWBIN/xzdec.exe "$BIN"
# Below added 3/25/2016
cp $USRBIN/msys-icudata59.dll "$BIN"
cp $USRBIN/msys-icui18n59.dll "$BIN"
# Above added 3/25/2016

# Below added 3/6/2017. This requires installation of msys2 packages msys/unzip and zip,
# for which see windows\buildenvWin64.sh.
echo "Copying zip and unzip..."
cp $USRBIN/zip.exe "$BIN"
cp $USRBIN/unzip.exe "$BIN"
cp $USRBIN/msys-bz2-1.dll "$BIN"
# Above added 3/6/2017

# Below added 3/20/2017
cp $MINGWBIN/gspawn-$WINBITS-helper "$BIN"
# This should have created the .exe version of the above-named file
# Above added 3/20/2017

# Below added 3/29/2016. This is critical for crash-free operation,
# for example, when opening Related Verses or References windows that
# use libwebkit.
echo "Copying all-important font configuration..."
mkdir -v -p "$ETC/fonts"
cp -R "/$MINGWDIR/etc/fonts/fonts.conf" "$ETC/fonts"
# We could do the following...
#cp -R "/$MINGWDIR/etc/fonts/conf.d" "$ETC/fonts"
# ...but the following has a larger selection of fonts
cp -R "/$MINGWDIR/share/fontconfig/conf.avail/" "$ETC/fonts/"
# Match the directory name to what is already in the fonts.conf file
mv -f "$ETC/fonts/conf.avail" "$ETC/fonts/conf.d"
# Above added 3/29/2016

# The DLL ieshims.dll is a special case. depends.exe says we need it, but we don't.

echo "Copying themes and engines..."
mkdir -v -p "$SHARE/themes"
cp -R $THEMES/* "$SHARE/themes/"
mkdir -v -p "$LIB/gtk-2.0"
cp -R $ENGINES/* "$LIB/gtk-2.0/"

echo "Copying templates, etc. to $SHARE/bibledit..."
mkdir -v -p "$SHARE/bibledit"
cp -R templates/* "$SHARE/bibledit"
cp -R BiblesInternational/* "$SHARE/bibledit"
# Added next line 4/4/2016
cp -R kjv/kjv.sql "$SHARE/bibledit"
cp -R pix/* "$SHARE/bibledit"
cp -R usfm/usfm.sty "$SHARE/bibledit"
cp -R doc/site "$SHARE/bibledit"
cp -R doc/index.html "$SHARE/bibledit"
cp -R doc/menus.pl "$SHARE/bibledit"
cp -R doc/retrieval.pl "$SHARE/bibledit"
cp -R doc/style.css "$SHARE/bibledit"
cp -R doc/site.xml "$SHARE/bibledit"
# Added 3/14/2018 to support new reference bibles in Analysis window
mkdir -v -p "$SHARE/bibledit/bibles"
BIBLES="$SHARE/bibledit/bibles"
#mkdir -v -p "$BIBLES/sblgnt"
#mkdir -v -p "$BIBLES/byzascii"
#mkdir -v -p "$BIBLES/engleb"
cp -R bibles/sblgnt "$BIBLES/"
cp -R bibles/byzascii "$BIBLES/"
cp -R bibles/engmtv "$BIBLES/"
cp -R bibles/engleb "$BIBLES/"
cp -R bibles/engnet "$BIBLES/"
# Added 3/28/2018 to support new cross-reference database in Analysis window
cp -R BiblesInternational/bi.crf "$BIBLES/"

# Added 2/21/2017
echo "Setting up usr/bin and tmp"
mkdir -v -p "$DEST_TMP"
mkdir -v -p "$DEST_USRBIN"
cp $USRBIN/sh.exe "$DEST_USRBIN"

echo "Fetching bwoutpost.exe and installing to $BIN"
# Remove old version if there is one in this directory
rm -f bwoutpost.exe
wget http://fbcaa.org/bibledit/bwoutpost.exe
mv -f bwoutpost.exe "$BIN"

# This copies the necessary materials to a place where
# Windows utilities can access it, then generates an installer.
# 6/9/16 - snoeberg
if [ "$makeinstall" = "1" ]; then
	if [ "$PROGRAMFILES" = "Program Files" ]; then
		BIT="64"
	fi
	TEMPDIR="/c/tempBibleEditFolderForInstall"
	mkdir "$TEMPDIR"
	# Change to that directory, download some helpful components, and change back
	WORKDIR=`pwd`
	cd "$TEMPDIR"
	wget http://www.fbcaa.org/bibledit/InstallationBuilder/7z.dll
	wget http://www.fbcaa.org/bibledit/InstallationBuilder/7z.exe
	wget http://www.fbcaa.org/bibledit/InstallationBuilder/7z.sfx
	wget http://www.fbcaa.org/bibledit/InstallationBuilder/7zCon.sfx
	wget http://www.fbcaa.org/bibledit/InstallationBuilder/7zFM.exe
	wget http://www.fbcaa.org/bibledit/InstallationBuilder/7zG.exe
	wget http://www.fbcaa.org/bibledit/InstallationBuilder/7-zip.chm
	wget http://www.fbcaa.org/bibledit/InstallationBuilder/7-zip.dll
	cd $WORKDIR
	WORKDIR=''
	pwd
	# Ok, we are back...
	cp windows/InstallationBuilder/* "$TEMPDIR"
	echo 'Making icon.'
	# No native bash for creating windows icon, using powershell
	PSFILE="C:\\tempBibleEditFolderForInstall\\makeicon.ps1"
	# Powershell v 2 does not quit after script is complete...work around by wrapping it with call to cmd
	#/c/Windows/System32/WindowsPowerShell/v1.0/powershell.exe -ExecutionPolicy Bypass -NoLogo -NonInteractive -NoProfile -file $PSFILE -version $VERSION -bit $BIT
    cmd /c "START /WAIT c:\Windows\System32\WindowsPowerShell\v1.0\powershell.exe -ExecutionPolicy Bypass -NoLogo -NonInteractive -NoProfile -file $PSFILE -version $VERSION -bit $BIT"
    echo 'Compressing...'
	"$TEMPDIR/7z.exe" a -sfx "C:\\tempBibleEditFolderForInstall\\Bibledit.exe" "C:\\$PROGRAMFILES\\Bibledit-$VERSION"
	
	##Creating the settings file for the install. This gets a little messy.
	SEDFILE="/c/tempBibleEditFolderForInstall/Install-BiblEdit.SED"
	rm $SEDFILE -f
	echo '[Version]' >> "$SEDFILE"
echo 'Class=IEXPRESS' >> "$SEDFILE"
echo 'SEDVersion=3' >> "$SEDFILE"
echo '[Options]' >> "$SEDFILE"
echo 'PackagePurpose=InstallApp' >> "$SEDFILE"
echo 'ShowInstallProgramWindow=0' >> "$SEDFILE"
echo 'HideExtractAnimation=0' >> "$SEDFILE"
echo 'UseLongFileName=1' >> "$SEDFILE"
echo 'InsideCompressed=0' >> "$SEDFILE"
echo 'CAB_FixedSize=0' >> "$SEDFILE"
echo 'CAB_ResvCodeSigning=0' >> "$SEDFILE"
echo 'RebootMode=N' >> "$SEDFILE"
echo 'InstallPrompt=%InstallPrompt%' >> "$SEDFILE"
echo 'DisplayLicense=%DisplayLicense%' >> "$SEDFILE"
echo 'FinishMessage=%FinishMessage%' >> "$SEDFILE"
echo 'TargetName=%TargetName%' >> "$SEDFILE"
echo 'FriendlyName=%FriendlyName%' >> "$SEDFILE"
echo 'AppLaunched=%AppLaunched%' >> "$SEDFILE"
echo 'PostInstallCmd=%PostInstallCmd%' >> "$SEDFILE"
echo 'AdminQuietInstCmd=%AdminQuietInstCmd%' >> "$SEDFILE"
echo 'UserQuietInstCmd=%UserQuietInstCmd%' >> "$SEDFILE"
echo 'SourceFiles=SourceFiles' >> "$SEDFILE"
echo '[Strings]' >> "$SEDFILE"
echo 'InstallPrompt=' >> "$SEDFILE"
echo 'DisplayLicense=' >> "$SEDFILE"
echo 'FinishMessage=Installation Complete!' >> "$SEDFILE"
echo "TargetName=C:\\tempBibleEditFolderForInstall\\BiblEdit-$VERSION.exe" >> "$SEDFILE"
echo 'FriendlyName=BiblEdit' >> "$SEDFILE"
echo "AppLaunched=cmd /c autoelevate.cmd $VERSION" >> "$SEDFILE"
echo 'PostInstallCmd=cmd /c pausescript.bat' >> "$SEDFILE"
echo 'AdminQuietInstCmd=' >> "$SEDFILE"
echo 'UserQuietInstCmd=' >> "$SEDFILE"
echo 'FILE0="Bibledit.exe"' >> "$SEDFILE"
echo 'FILE1="autoelevate.cmd"' >> "$SEDFILE"
echo 'FILE2="pausescript.bat"' >> "$SEDFILE"
echo '[SourceFiles]' >> "$SEDFILE"
echo 'SourceFiles0=C:\tempBibleEditFolderForInstall\' >> "$SEDFILE"
echo '[SourceFiles0]' >> "$SEDFILE"
echo '%FILE0%=' >> "$SEDFILE"
echo '%FILE1%=' >> "$SEDFILE"
echo '%FILE2%=' >> "$SEDFILE"
	
	/c/Windows/System32/iexpress.exe //N "C:\\tempBibleEditFolderForInstall\\Install-BiblEdit.SED"
	mv "$TEMPDIR/Bibledit-$VERSION.exe" "windows/Bibledit-$VERSION.exe"
	echo "Installer Bibledit-$VERSION.exe has been created. Output exe is in windows/Bibledit-$VERSION.exe"
	rm "$TEMPDIR" -rf
	exit 0;
fi

echo "Done..."
