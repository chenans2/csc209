CFLAGS = -Wall -std=c99 -Werror

mapreduce: master.o mapworker.o reduceworker.o linkedlist.o
	gcc $(CFLAGS) -o mapreduce master.o mapworker.o reduceworker.o

master.o: master.c mapreduce.h
	gcc $(CFLAGS) -c master.c

mapworker.o: mapworker.c mapreduce.h word_freq.o
	gcc $(CFLAGS) -c mapworker.c
	
reduceworker.o: reduceworker.c mapreduce.h word_freq.o
	gcc $(CFLAGS) -c reduceworker.c
	
linkedlist.o: linkedlist.c linkedlist.h
	gcc $(CFLAGS) -c linkedlist.c
	
word_freq.o: word_freq.c
	gcc $(CFLAGS) -c word_freq.c

clean: 
	rm mapreduce *.o *.out
