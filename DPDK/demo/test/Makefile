# SPDX-License-Identifier: BSD-3-Clause
# Copyright(c) 2010-2014 Intel Corporation

# binary name
SERVER = server
CLIENT = client
TEST = test

# all source are stored in SRCS-y
SRCS-y := server.c
SRCS-z := client.c
SRCS-x := test.c

PKGCONF ?= pkg-config

# Build using pkg-config variables if possible
ifneq ($(shell $(PKGCONF) --exists libdpdk && echo 0),0)
$(error "no installation of DPDK found")
endif

.PHONY: server client
server: build/$(SERVER)-static
	@ln -sf $(SERVER)-static build/$(SERVER)
client: build/$(CLIENT)-static
	@ln -sf $(CLIENT)-static build/$(CLIENT)
test: build/$(TEST)-static
	@ln -sf $(TEST)-static build/$(TEST)

PC_FILE := $(shell $(PKGCONF) --path libdpdk 2>/dev/null)
CFLAGS += -O3 $(shell $(PKGCONF) --cflags libdpdk)
# Add flag to allow experimental API as l2fwd uses rte_ethdev_set_ptype API
CFLAGS += -DALLOW_EXPERIMENTAL_API
CFLAGS += -g -Wno-address-of-packed-member
LDFLAGS_SHARED = $(shell $(PKGCONF) --libs libdpdk)
LDFLAGS_STATIC = $(shell $(PKGCONF) --static --libs libdpdk)

ifeq ($(MAKECMDGOALS),static)
# check for broken pkg-config
ifeq ($(shell echo $(LDFLAGS_STATIC) | grep 'whole-archive.*l:lib.*no-whole-archive'),)
$(warning "pkg-config output list does not contain drivers between 'whole-archive'/'no-whole-archive' flags.")
$(error "Cannot generate statically-linked binaries with this version of pkg-config")
endif
endif

build/$(SERVER)-static: $(SRCS-y) Makefile $(PC_FILE) | build
	@$(CC) $(CFLAGS) $(SRCS-y) -o $@ $(LDFLAGS) $(LDFLAGS_SHARED)

build/$(CLIENT)-static: $(SRCS-z) Makefile $(PC_FILE) | build
	@$(CC) $(CFLAGS) $(SRCS-z) -o $@ $(LDFLAGS) $(LDFLAGS_STATIC)

build/$(TEST)-static: $(SRCS-x) Makefile $(PC_FILE) | build
	@$(CC) $(CFLAGS) $(SRCS-x) -o $@ $(LDFLAGS) $(LDFLAGS_STATIC)

build:
	@mkdir -p $@

.PHONY: clean
clean:
	rm -f build/$(SERVER) build/$(SERVER)-static build/$(SERVER)-shared
	rm -f build/$(CLIENT) build/$(CLIENT)-static build/$(CLIENT)-shared
	rm -f build/$(TEST) build/$(TEST)-static build/$(TEST)-shared
	test -d build && rmdir -p build || true
