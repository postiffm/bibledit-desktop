#dist_pkgdata_DATA = *.html
EXTRA_DIST = site
CLEANFILES = *~

install-data-local:
	@rm -rf site
	@mkdir site
	@yelp-build html -o site/ C/*.page
	@mkdir -p $(DESTDIR)$(prefix)/$(packagedatadir)
	@cp -r site $(DESTDIR)$(prefix)/$(packagedatadir)