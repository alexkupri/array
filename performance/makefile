amber: main.o
	g++  -o"amber"  ./main.o   

main.o: main.cpp alex_kupriianov_array.h alex_kupriianov_array2.h
	g++ -O3 -Wall -c -fmessage-length=0 -o"main.o" "main.cpp"

clean:
	rm amber main.o

