# Get variables from Makefile.defs

include ../makefile.defs


.DEFAULT: all

.SUFFIXES: .stats_o
.SUFFIXES: .cpp

SOURCES = ../core/stats/stats.cpp ../core/stats/measurement.cpp ../core/stats/stats_calc.cpp $(wildcard ../core/debug/*.cpp)

%.stats_o: %.cpp
	$(GCC) $(GCC_OPTFLAGS) $(GCC_LANGFLAGS) -isystem $(BOOST_INCLUDE) -isystem $(SIMICS_INCLUDE) -DSELECTED_DEBUG=dev -DTARGET_PLATFORM=v9 -I.. -c $^ -o $@

all: stat-manager stat-collapse stat-sample

stat-manager: $(subst .cpp,.stats_o,$(SOURCES) stat-manager.cpp )
	echo $^
	$(GCC) $(GCC_OPTFLAGS) $(INCLUDES) -DSELECTED_DEBUG=dev -DTARGET_PLATFORM=v9 -I.. $^ -L../core/lib $(BOOST_LIBRARIES) -pthread -static -o $@

stat-collapse: $(subst .cpp,.stats_o,$(SOURCES) stat-collapse.cpp )
	echo $^
	$(GCC) $(GCC_OPTFLAGS) $(INCLUDES) -DSELECTED_DEBUG=dev -DTARGET_PLATFORM=v9 -I.. $^ -L../core/lib $(BOOST_LIBRARIES) -pthread -static -o $@

stat-sample: $(subst .cpp,.stats_o,$(SOURCES) stat-sample.cpp )
	echo $^
	$(GCC) $(GCC_OPTFLAGS) $(INCLUDES) -DSELECTED_DEBUG=dev -DTARGET_PLATFORM=v9 -I.. $^ -L../core/lib $(BOOST_LIBRARIES) -pthread -static -o $@


clean:
	rm -f *.stats_o stat-manager stat-sample stat-collapse $(subst .cpp,.stats_o,$(SOURCES) )
