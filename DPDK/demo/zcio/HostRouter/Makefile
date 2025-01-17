# SPDX-License-Identifier: BSD-3-Clause
# Copyright(c) 2010-2014 Intel Corporation

# binary name
APP = router

# all source are stored in SRCS-y
SRC_DIR = src
SRCS-y := $(wildcard $(SRC_DIR)/*.c $(SRC_DIR)/**/*.c)

PKGCONF ?= pkg-config

# Build using pkg-config variables if possible
ifneq ($(shell $(PKGCONF) --exists libdpdk && echo 0),0)
$(error "no installation of DPDK found")
endif

all: shared
.PHONY: shared static
shared: build/$(APP)-shared
	@ln -sf $(APP)-shared build/$(APP)
static: build/$(APP)-static
	@ln -sf $(APP)-static build/$(APP)

PC_FILE := $(shell $(PKGCONF) --path libdpdk 2>/dev/null)
CFLAGS += $(shell $(PKGCONF) --cflags libdpdk)
# Add flag to allow experimental API as l2fwd uses rte_ethdev_set_ptype API
CFLAGS += -O3 -g -gdwarf-2 -Wno-address-of-packed-member
LDFLAGS_SHARED += $(shell $(PKGCONF) --libs libdpdk)
LDFLAGS_STATIC += $(shell $(PKGCONF) --static --libs libdpdk)
LDFLAGS_STATIC += -Wl,--no-whole-archive -lrt -lm -ldl -lcrypto -pthread -lnuma

ifeq ($(MAKECMDGOALS),static)
# check for broken pkg-config
ifeq ($(shell echo $(LDFLAGS_STATIC) | grep 'whole-archive.*l:lib.*no-whole-archive'),)
$(warning "pkg-config output list does not contain drivers between 'whole-archive'/'no-whole-archive' flags.")
$(error "Cannot generate statically-linked binaries with this version of pkg-config")
endif
endif

build/$(APP)-shared: $(SRCS-y) Makefile $(PC_FILE)  | build
	cc $(CFLAGS) $(SRCS-y) -o $@ $(LDFLAGS) $(LDFLAGS_SHARED)

build/$(APP)-static: $(SRCS-y) Makefile $(PC_FILE) | build
	cc $(CFLAGS) $(SRCS-y) -o $@ $(LDFLAGS) $(LDFLAGS_STATIC)

build:
	@mkdir -p $@

.PHONY: clean
clean:
	@rm -f build/$(APP) build/$(APP)-static build/$(APP)-shared
	@test -d build && rmdir -p build || true
