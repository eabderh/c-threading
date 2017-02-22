# ---- FILE HEADER ----------------------------------------------------------
# project: project2
# file: makefile
# student/author: elias abderhalden
# date: 2014-10-17
# ---------------------------------------------------------------------------
# class: ece3220 fall 2014
# instructor: jacob sorber
# assignment: project #2
# purpose: User Mode Thread Library
# ---------------------------------------------------------------------------
# notes:	commands
# 			make - build programs
# 			make clean - delete object files, executable, and core
# ---------------------------------------------------------------------------

cc = gcc
ccf = -g -Wall
ccl = -lm
lb = ar
lbf = -cvq

trg = libmythreads.a


project2 : $(trg)

$(trg) : libmythreads.o
	$(lb) $(lbf) $(trg) libmythreads.o 

%.o : %.c
	$(cc) $(ccf) -c -o $@ $<



cl : cla clt
	rm -f core *.o test
clt :
	rm -f *test
cla :
	rm -f *.a


