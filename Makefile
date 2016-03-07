all:
	gcc -w -o sender sender.c
	gcc -w -o receiver receiver.c

warning:
	gcc -o server server.c
	gcc -o receiver receiver.c

clean:
	rm sender
	rm receiver