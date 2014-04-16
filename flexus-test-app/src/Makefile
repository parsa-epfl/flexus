
PROGNAME = filter

CC = gcc
EXT = .c

#add the required paths, e.g.
LIBDIRS = ../netpbm/lib
INCDIRS = ../netpbm/lib ../netpbm
CFLAGS = -g3 -O3 -Wall
CPPFLAGS = -D_GNU_SOURCE
LDFLAGS = ../netpbm/lib/libnetpbm.a
LIBS = pthread

##########################################################

CXX = $(CC)
CXXFLAGS = $(CFLAGS)

LDFLAGS += $(addprefix -L,$(LIBDIRS)) 
CPPFLAGS += $(addprefix -I,$(INCDIRS))

.PHONY: all strip clean depends docs pdf epsfigs

sources := $(subst ./,,$(shell find . -name \*$(EXT)))
deps := $(addprefix .deps/,$(sources:$(EXT)=.d))
docsources := $(wildcard *$(TEX))
figs := $(patsubst %.fig,%.eps,$(wildcard *.fig))
objects := $(sources:$(EXT)=.o)

all: $(deps) $(PROGNAME) docs

$(PROGNAME): $(objects)
	@echo "Building executable ($@)"
	$(CC) -o $@ $(filter-out %.d,$^) $(LDFLAGS) $(addprefix -l,$(LIBS))
	@size $@
	@echo "Done"

strip: $(PROGNAME)
	@echo "Before:"
	@size $<
	@strip $<
	@echo "After:"
	@size $<
	@echo "Done"

%.o: %$(EXT)
	$(CXX) -o $@ -c $(CPPFLAGS) $(CXXFLAGS) $<

docs: $(patsubst %.tex,%.dvi,$(docsources))

pdf: $(patsubst %.tex,%.pdf,$(docsources))

epsfigs: $(figs)
	@echo "Rebuilt EPS files from FIGs."

%.dvi: %.tex $(wildcard *.bib) $(wildcard *.eps) epsfigs
	latex $(patsubst %.tex,%,$<)
	bibtex $(patsubst %.tex,%,$<)
	latex $(patsubst %.tex,%,$<)
	latex $(patsubst %.tex,%,$<)

%.ps: %.dvi
	dvips -Ppdf -tletter $<

%.pdf: %.ps
	ps2pdf $<

%.eps: %.fig
	fig2dev -L eps $^ $@

clean:
	rm -f $(PROGNAME) $(PROGNAME).core core
	rm -f $(objects)
	csh -c "rm -f $(patsubst %.tex,%,$(docsources)).{dvi,ps,pdf,aux,log,bbl,blg}"
	rm -f $(patsubst %.fig,%.eps,$(figs))
	@echo "Done"

redep:
	@echo "Removing dependency files"
	@rm -fr .deps

veryclean: clean redep

backup: veryclean
	@echo "Creating backup..."
	@mkdir -p ../backups
	@cp -r . "../backups/$(PROGNAME).`date`"

file = $(patsubst .deps/%.d,%,$@)
$(deps):
	@echo "Generating dependencies ($@)"
	@mkdir -p $(shell dirname $@)
	@sh -ec "$(CC) -MM $(CPPFLAGS) $(file)$(EXT) | sed '1s|^.*:|$@ $(file).o:|g' > $@"

-include $(deps) /dev/null
