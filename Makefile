
gen_table:	gen_table.c 
	gcc -Wall -g -o gen_table gen_table.c

tmds_table.h:	gen_table tmds_decoder.vhdl
	./gen_table > tmds_table.h

hist:	hist.c Makefile tmds_table.h
	gcc -Wall -g -o hist hist.c
