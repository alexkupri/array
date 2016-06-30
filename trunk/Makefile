FLAGS =	-O2 -Wall -fmessage-length=0

OBJS =		main.o

TARGET =	btree

$(TARGET):	$(OBJS)
	g++ -o $(TARGET) $(OBJS)

main.o	:	main.cpp btree_seq.h btree_seq2.h
	g++ -c $(FLAGS) main.cpp

all:	$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET) *.txt
