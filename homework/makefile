
GCCOPT = gcc -O2 -funroll-loops
GCCDEB = gcc -ggdb -Wall #-pedantic

#CCC = $(GCCOPT)
CCC = $(GCCDEB)

#GRB = /opt/gurobi/gurobi650/linux64
GRB = /mnt/sda5/gurobi651/linux64

GRBINCDIR = $(GRB)/include
GRBLIBDIR = $(GRB)/lib

LIBS = -lm -lpthread -lgurobi65

GRBCFLAGS = -I $(GRBINCDIR)
GRBLNFLAGS = -L $(GRBLIBDIR) $(LIBS)

CCCFLAGS = $(GRBCFLAGS)
CCCLNFLAGS = $(GRBLNFLAGS)


BDIR = bin
ODIR = obj

ROB = $(BDIR)/rob
ROB_OBJ = $(ODIR)/robust.o $(ODIR)/robustmain.o

PFREBSIM = $(BDIR)/pfrebsim
PFREBSIM_OBJ = $(ODIR)/pfload.o $(ODIR)/pfworker.o $(ODIR)/pfsimul.o $(ODIR)/utilities.o $(ODIR)/rebengine.o

REB = $(BDIR)/reb
REB_OBJ = $(ODIR)/rebengine.o $(ODIR)/rebmain.o $(ODIR)/utilities.o

all: $(ROB) $(REB) $(PFREBSIM)

$(ODIR)/%.o: src/%.c
	@echo compiling $*.c with $(CCC) $(CCCFLAGS)
	@$(CCC) $(CCCFLAGS) -c $< -o $@

$(ROB): $(ROB_OBJ)
	$(CCC) -o $(ROB) $(ROB_OBJ) $(CCCLNFLAGS)

$(REB): $(REB_OBJ) 
	$(CCC) -o $(REB) $(REB_OBJ) $(CCCLNFLAGS)

$(PFREBSIM): $(PFREBSIM_OBJ)
	$(CCC) -o $(PFREBSIM) $(PFREBSIM_OBJ) $(CCCLNFLAGS)

clean:
	rm $(ROB) $(REB) $(PFREBSIM) -f
	rm $(ODIR)/* -f

