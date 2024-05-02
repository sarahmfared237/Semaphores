build: 
	gcc -pthread sem.c -o sem
	
clean:
	rm -f sem
