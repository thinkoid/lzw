# -*- mode: makefile; -*-

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

TARGET = lzw

all: $(TARGET)

DEPS = $(patsubst %.o,$(DEPENDDIR)/%.d,$(OBJS))
-include $(DEPS)

$(DEPENDDIR)/%.d: %.cc $(DEPENDDIR)
	$(CXX) $(CPPFLAGS) $(CFLAGS) $(DEPENDFLAGS) $< >$@

$(DEPENDDIR):
	@[ ! -d $(DEPENDDIR) ] && mkdir -p $(DEPENDDIR)

%: %.cc

%.o: %.cc
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $<

lzw: $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

clean:
	@rm -rf $(TARGET) $(OBJS)

realclean:
	@rm -rf $(TARGET) $(OBJS) $(DEPENDDIR)

install: lzw
	install $< $(INSTALLDIR)
