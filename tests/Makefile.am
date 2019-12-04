DISTCLEANFILES = \
	atconfig \
	$(NULL)

EXTRA_DIST = \
	test_nodes.vty

if ENABLE_EXT_TESTS
python-tests: $(BUILT_SOURCES)
	$(MAKE) vty-test
	osmotestvty.py -p $(abs_top_srcdir) -w $(abs_top_builddir) -v
	osmotestconfig.py -p $(abs_top_srcdir) -w $(abs_top_builddir) -v
else
python-tests: $(BUILT_SOURCES)
	echo "Not running python-based tests (determined at configure-time)"
endif

# To update the VTY script from current application behavior,
# pass -u to vty_script_runner.py by doing:
#   make vty-test U=-u
vty-test:
	osmo_verify_transcript_vty.py -v \
		-n osmo-e1-recorder -p 4444 \
		-r "$(top_builddir)/src/osmo-e1-recorder -c $(top_srcdir)/doc/examples/osmo-e1-recorder.cfg" \
		$(U) $(srcdir)/*.vty

check-local:
	$(MAKE) $(AM_MAKEFLAGS) python-tests
