#CC=g++
#build_env.mk sets CC.

# $(call log) comes from logging.mk
include ../../logging.mk

# You can include build_env.mk into your own
# Makefile to get the cross compilation flags
include ../../build_env.mk

.PHONY: all
all: log-build histogram tme

.PHONY: build-all
build-all: log-build histogram tme

log-build:
	@$(log_build)

log-install:
	@$(log_install)

histogram: src/histogram/histogram.cc src/shared.cc src/shared.hh src/params.hh src/util.hh
	$(CC) $(CCFLAGS) -DTIMING_PTHREAD $(LDFLAGS) -o $@ src/histogram/histogram.cc src/shared.cc
	codesign -s - histogram

tme: src/histogram/experiment-1.cc src/shared.cc src/shared.hh src/params.hh src/util.hh
	$(CC) $(CCFLAGS) -DTIMING_PTHREAD $(LDFLAGS) -o $@ src/histogram/experiment-1.cc src/shared.cc
	codesign -s - tme

hammering: src/hammering/hammering.cc src/shared.cc src/shared.hh src/params.hh src/util.hh
	$(CC) $(CCFLAGS) $(LDFLAGS) -o $@ src/hammering/hammering.cc src/shared.cc
	codesign -s - hammering



# Removed before the copy, @$(log_install) \n cp hello ${CRYPTEX_BIN_DIR} \n cp hello.plist ${CRYPTEX_LAUNCHD_DIR}
.PHONY: install
install:  build-all log-install install-histogram install-tme

install-histogram: histogram histogram.plist 
	cp histogram ${CRYPTEX_BIN_DIR}
	cp histogram.plist ${CRYPTEX_LAUNCHD_DIR}

install-tme: tme tme.plist 
	cp tme ${CRYPTEX_BIN_DIR}
	cp tme.plist ${CRYPTEX_LAUNCHD_DIR}

.PHONY: clean
clean: clean-histogram clean-tme

clean-histogram:
	rm -f histogram
	rm -f ${CRYPTEX_BIN_DIR}/histogram

clean-tme:
	rm -f tme
	rm -f ${CRYPTEX_BIN_DIR}/tme