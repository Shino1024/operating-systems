all:
	gcc -Wall -std=c99 barber.c fifo.c -o barber
	gcc -Wall -std=c99 client.c fifo.c -o client
	gcc -Wall -std=c99 client_factory.c -o client_factory

clean:
	rm barber client client_factory