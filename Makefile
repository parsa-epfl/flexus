include makefile.defs

.PHONY: core install stat-manager

stat-manager:
	@cd stat-manager ; $(MAKE) $(SILENT_MAKE) -f Makefile all

ifndef FLEXUS_ROOT

#determine what FLEXUS_ROOT should be
.DEFAULT core:
	$(MAKE) $(SILENT_MAKE) FLEXUS_ROOT=`pwd` $@

list_targets:
	$(MAKE) $(SILENT_MAKE) FLEXUS_ROOT=`pwd` $@

install:
	$(MAKE) $(SILENT_MAKE) FLEXUS_ROOT=`pwd` -f makefile.install install

uninstall:
	$(MAKE) $(SILENT_MAKE) FLEXUS_ROOT=`pwd` -f makefile.install uninstall

else
# We know what Flexus root is.

install:
	$(MAKE) $(SILENT_MAKE) FLEXUS_ROOT=`pwd` -f makefile.install install

ifndef SETUP_OK
# Check the Flexus setup to make sure all paths are correct, Simics, and Boost
# are installed correctly.

.DEFAULT core:
	$(MAKE) $(SILENT_MAKE) -f makefile.checksetup
	$(MAKE) $(SILENT_MAKE) SETUP_OK=true $@

else
# We have checked the Flexus setup and it is ok to proceed with the build

ifndef TARGET_PARSED
# We need to parse the target name
# We break the target into a TARGET and a TARGET_SPEC, breaking up on the first dash.  All dashes are stripped out of both

.DEFAULT core:
	$(MAKE) $(SILENT_MAKE) TARGET_PARSED=true $(word 1,$(subst -, ,$@)) "TARGET_OPTIONS=$(strip $(filter-out $(word 1,$(subst -, ,$@)),$(subst -, ,$@)))"

else
# We have split the target name and spec.  Invoke the appropriate makefile
# to build whatever the target is

# Pick the make target apart to see what it is.
# It could be:
#     - a simulator, in which case:
#		$@ names a directory under FLEXUS_ROOT/simulators
#     - a special make target, in which case:
#	      Makefile.$@ exists and supports that target
# Otherwise, it is an error.  In the case of an error, we print out all legal targets

.DEFAULT core:
	if [[ -e makefile.$@ ]] ; then \
		$(MAKE) $(SILENT_MAKE) -f makefile.$@ $@ ; \
	elif [[ -d $(SIMULATORS_DIR)/$@ ]] ; then \
		$(MAKE) $(SILENT_MAKE) -f makefile.simulators $@ ; \
	elif [[ -d $(COMPONENTS_DIR)/$@ ]] ; then \
		$(MAKE) $(SILENT_MAKE) -f makefile.components $@ ; \
	else \
		if [[ "$@" != "list_targets" ]] ; then \
			echo "$@ is not a valid simulator or component." ; \
		fi ; \
		echo "Supported simulators:" ; \
		ls -ICVS $(SIMULATORS_DIR) ; \
		echo "Supported components:" ; \
		ls -ICVS $(COMPONENTS_DIR) ; \
	fi

endif
endif
endif
