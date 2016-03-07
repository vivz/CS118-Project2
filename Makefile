portnum = 14000
window_size = 5
p_loss = 0.0
p_corrupt = 0.0
host = "localhost"
filename = "small.txt"

sender_args = $(portnum) $(window_size) $(p_loss) $(p_corrupt)
receiver_args = $(host) $(portnum) $(filename) $(p_loss) $(p_corrupt)

all:
	gcc -w -o sender sender.c
	gcc -w -o receiver receiver.c
	mkdir -p receiver_dir
	mv receiver receiver_dir

warning:
	gcc -o server server.c
	gcc -o receiver receiver.c
	mkdir -p receiver_dir
	mv receiver receiver_dir

runsender:
	./sender $(sender_args)

runreceiver:
	./receiver_dir/receiver $(receiver_args)

clean:
	rm sender
	rm -rf receiver_dir