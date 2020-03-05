.RECIPEPREFIX := $(.RECIPEPREFIX) 
SHELL = /bin/bash
.DELETE_ON_ERROR :

binary_name := yback
source_name := yback.cpp
header_name := header/misc.h header/color.h header/getopt_class.h

.PHONY: all debug install uninstall clean

all : CXXFLAGS += -O3 -D NDEBUG
all : $(binary_name)

debug : CXXFLAGS += -g
debug : $(binary_name)

$(binary_name) : $(source_name) $(header_name)
    g++ -o $@ $(CXXFLAGS) $<

install : $(binary_name)
    $(if $(prefix),\
    cp $^ $(prefix),\
    $(error Please define `prefix` variable.))

clean :
    rm $(binary_name)

uninstall :
    $(if $(prefix),\
    rm $(prefix)/$(binary_name),\
    $(error Please define `prefix` variable.))

$(binary_name).out : $(source_name) $(header_name)
    g++ -o $@ -O3 -D NDEBUG $<

