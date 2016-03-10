portnum = 14000
window_size = 10
p_loss_sender = 0.01
p_corrupt_sender = 0.01
p_loss_receiver = 0.01
p_corrupt_receiver = 0.01

host = "localhost"
filename = large.txt
cwd = $(shell pwd)
sender_args = $(portnum) $(window_size) $(p_loss_sender) $(p_corrupt_sender)
receiver_args = $(host) $(portnum) $(filename) $(p_loss_receiver) $(p_corrupt_receiver)

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    TERMINALCOMMAND = gnome-terminal -e "bash sender.sh"
    SHELL = bash
endif
ifeq ($(UNAME_S),Darwin)
    TERMINALCOMMAND = open -a Terminal.app sender.sh
    SHELL = zsh
endif


all:
	gcc -w -o sender -g sender.c
	gcc -w -o receiver -g receiver.c

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
	echo -e "#!/bin/sh\ncd $(cwd)\nmake rr\n$(SHELL)" > sender.sh 
	$(TERMINALCOMMAND)
	chmod +x sender.sh
	make rs
	diff $(filename) $(addsuffix _copy, $(filename))

clean:
	rm sender
	rm receiver
	rm *_copy
	rm sender.sh
