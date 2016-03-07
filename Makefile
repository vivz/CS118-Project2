all:
    gcc -w -o server server.c
    gcc -w -o client client.c

clean:
    rm server