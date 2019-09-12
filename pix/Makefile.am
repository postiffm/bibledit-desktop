iconsdir = $(datadir)/icons/hicolor

nobase_dist_icons_DATA = \
	16x16/apps/bibledit-desktop.png \
	48x48/apps/bibledit-desktop.png \
	96x96/apps/bibledit-desktop.png

appicondir = $(datadir)/pixmaps
appicon_DATA = bibledit-desktop.xpm

dist_pkgdata_DATA = *.xpm *.jpeg *.ico *.svg *.gif *.png
CLEANFILES = *~

gtk_update_icon_cache = gtk-update-icon-cache -f -t $(iconsdir)

install-data-hook: update-icon-cache
uninstall-hook: update-icon-cache
update-icon-cache:
	@-if test -z "$(DESTDIR)"; then \
		echo "Updating GTK icon cache."; \
		$(gtk_update_icon_cache); \
	else \
		echo "*** Icon cache not updated. After (un)install, run this:"; \
		echo "*** $(gtk_update_icon_cache)"; \
	fi
