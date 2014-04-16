
#COPTS=-I.. -Wall -g -O0 -DNS_STANDALONE #-pg -a 
COPTS=-I.. -Wall -g -O0 -DNS_STANDALONE -DLOG_RESULTS

PROGS=netshim torus 
PROGS+=kn truss # CMU-ONLY

all: $(PROGS)

OBJS = \
	../netcontainer.o \
	../netnode.o \
	../netcommon.o \
	../channel.o \
	../netswitch.o \
	regress.o \
	histogram.o

.cpp.o:
	$(CXX) -c $(COPTS) $(INCLUDE) -o $@ $<

.c.o:
	$(CXX) -c $(COPTS) $(INCLUDE) -o $@ $<

clean:
	rm -f *~ *.o $(TARGET) core $(OBJS) $(PROGS)

netshim: $(OBJS) main.o
	$(CXX) -o $@ $(COPTS) $+ $(LIBS)

torus:	torus.o
	$(CXX) -o $@ $(COPTS) $< $(LIBS)

topology:	topology.o
	$(CXX) -static -o $@ $(COPTS) $< $(LIBS)

# CMU-ONLY-BLOCK-BEGIN
kn:	kn.o
	$(CXX) -o $@ $(COPTS) $< $(LIBS)

truss:	truss.o $(OBJS)
	$(CXX) -o $@ $(COPTS) $+ $(LIBS)
# CMU-ONLY-BLOCK-END
