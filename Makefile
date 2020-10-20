# -*- mode: makefile; -*-
# Copyright (c) 2020- Thinkoid, LLC

OSNAME := $(shell uname)

VERSION := 0.1

WARNINGS =										\
	-Wno-unused-function						\
	-Wno-deprecated-declarations

CXXFLAGS = -g -O -std=c++2a -W -Wall $(WARNINGS)
CPPFLAGS = -I.

LDFLAGS =
LIBS = -lfmt

INSTALLDIR = ~/bin

DEPENDDIR = ./.deps
DEPENDFLAGS = -M

SRCS := $(wildcard *.cc)
OBJS := $(patsubst %.cc,%.o,$(SRCS))

TARGETS = lzw

all: $(TARGETS)

DEPS = $(patsubst %.o,$(DEPENDDIR)/%.d,$(OBJS))
-include $(DEPS)

$(DEPENDDIR)/%.d: %.cc $(DEPENDDIR)
	$(CXX) $(CPPFLAGS) $(CFLAGS) $(DEPENDFLAGS) $< >$@

$(DEPENDDIR):
	@[ ! -d $(DEPENDDIR) ] && mkdir -p $(DEPENDDIR)

%: %.cc

%.o: %.cc
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $<

%: %.o
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

clean:
	@rm -rf $(TARGETS) $(OBJS)

realclean:
	@rm -rf $(TARGETS) $(OBJS) $(DEPENDDIR)

install: lzw
	install $< $(INSTALLDIR)
