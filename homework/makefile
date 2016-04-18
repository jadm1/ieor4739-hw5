
GCCOPT = gcc -O2 -funroll-loops
GCCDEB = gcc -ggdb -Wall -pedantic

#CCC = $(GCCOPT)
CCC = $(GCCDEB)

#GRB = /opt/gurobi/gurobi650/linux64
GRB = /mnt/sda5/gurobi651/linux64

GRBINCDIR = $(GRB)/include
GRBLIBDIR = $(GRB)/lib

LIBS = -lm -lpthread -lgurobi65

CCCFLAGS = -I $(GRBINCDIR)
CCCLNFLAGS = -L $(GRBLIBDIR) $(LIBS)


BDIR = bin
ODIR = obj

ROB = $(BDIR)/rob
ROB_OBJ = $(ODIR)/robust.o $(ODIR)/robustmain.o


all: $(ROB)

$(ODIR)/%.o: src/%.c
	@echo compiling $*.c with $(CCC) $(CCCFLAGS)
	@$(CCC) $(CCCFLAGS) -c $< -o $@

$(ROB): $(ROB_OBJ)
	$(CCC) -o $(ROB) $(ROB_OBJ) $(CCCLNFLAGS)

clean:
	rm $(ROB) -f
	rm $(ODIR)/* -f

