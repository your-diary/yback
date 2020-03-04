.RECIPEPREFIX := $(.RECIPEPREFIX) 
SHELL = /bin/bash
.DELETE_ON_ERROR :

.PHONY: 

CXXFLAGS := -O3

%.out : %.cpp
    g++ $(CXXFLAGS) -o $@ $<

