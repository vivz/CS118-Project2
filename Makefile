portnum = 14000
window_size = 3
p_loss = 0.0
p_corrupt = 0.5
host = "localhost"
filename = small.txt
cwd = $(shell pwd)
sender_args = $(portnum) $(window_size) $(p_loss) $(p_corrupt)
receiver_args = $(host) $(portnum) $(filename) $(p_loss) $(p_corrupt)

all:
	gcc -w -o sender sender.c
	gcc -w -o receiver receiver.c

warning:
	gcc -o sender sender.c
	gcc -o receiver receiver.c

runsender:
	./sender $(sender_args)

runreceiver:
	./receiver $(receiver_args)

rs:
	make runsender

rr:
	make runreceiver

test:
	make
	echo "#!/bin/sh\ncd $(cwd)\nmake rr\nzsh" > sender.sh 
	open -a Terminal.app sender.sh
	chmod +x sender.sh
	make rs
	diff $(filename) $(addsuffix _copy, $(filename))

clean:
	rm sender
	rm receiver
	rm *_copy
	rm sender.sh