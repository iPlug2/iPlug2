# All libraries
LIBS := $(wildcard ../*.lib)

# All libraries without the 'entry' points ones
INTERNAL_LIBS := $(filter-out ../all.lib ../sf.lib ../stdfaust.lib, $(LIBS))

all: deps.pdf internal_deps.pdf

internal: internal_deps.pdf

help:
	@echo "============ Faust libraries tools ============"
	@echo "Available targets are:"
	@echo "  all (default) : compute a dependencies graph of all libraries using dot"
	@echo "  internal      : compute a dependencies graph of libraries (without 'entry' point ones) using dot"
	@echo "  clean         : remove output files"

deps.pdf: deps.dot

deps.dot: 
	echo "digraph impl { " > $@
	echo "margin=0.05;\n" >> $@
	egrep "=[ \t]*library|^import" $(LIBS) | sed -e 's/^\.\.\///' \
	| sed -e 's/:.*=[ \t]*library("/ -> /' | sed -e 's/:import("/ -> /' \
	| sed -e 's/"..*/;/' |sed -e 's/\.lib//g' | sed -e 's/\//_/g' >> $@
	echo "}" >> $@

internal_deps.pdf: internal_deps.dot

internal_deps.dot: 
	echo "digraph impl { " > $@
	echo "margin=0.05;\n" >> $@
	egrep "=[ \t]*library|^import" $(INTERNAL_LIBS) | sed -e 's/^\.\.\///' \
	| sed -e 's/:.*=[ \t]*library("/ -> /' | sed -e 's/:import("/ -> /' \
	| sed -e 's/"..*/;/' |sed -e 's/\.lib//g' | sed -e 's/\//_/g' >> $@
	echo "}" >> $@
	
clean:
	rm -f deps.pdf deps.dot internal_deps.pdf internal_deps.dot

test:
	echo $(LIBS) 

%.pdf: %.dot
	dot -Tpdf -o $@ $< 

