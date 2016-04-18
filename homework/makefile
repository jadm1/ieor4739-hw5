
GCCOPT = gcc -O2 -funroll-loops
GCCDEB = gcc -ggdb -Wall -pedantic

#CCC = $(GCCOPT)
CCC = $(GCCDEB)

CFLAGS = 


CCCLNFLAGS = $(LIBS)
CCCFLAGS = 

BDIR = bin
ODIR = obj


PROG = $(BDIR)/prog
PROG_OBJ = $(ODIR)/prog.o


all: $(PROG)

$(ODIR)/%.o: src/%.c
	@echo compiling $*.c with $(CCC) $(CCCFLAGS)
	@$(CCC) $(CCCFLAGS) -c $< -o $@

$(PROG): $(PROG_OBJ)
	$(CCC) $(CCCFLAGS) -o $(PROG) $(PROG_OBJ) $(CCCLNFLAGS)

clean:
	rm $(PROG) -f
	rm $(ODIR)/* -f

