TOPLEVEL ?= $(dir $(lastword $(MAKEFILE_LIST)))
CPPLINT ?= $(TOPLEVEL)/cpplint.py
PYTHON ?= python

LINT_SOURCES = \
	src/main.cpp \
	src/snapshot.cpp \
	src/snapshot.h \
	src/tasklist.h \
	src/win/tasklist.cpp \
	src/unix/tasklist.cpp

.PHONY: lint

FILTER = -legal/copyright

lint:
	cd $(TOPLEVEL) && $(PYTHON) $(CPPLINT) --filter=$(FILTER) $(LINT_SOURCES)
