all: daemon console

daemon: daemon/modules/* daemon/daemon.c
	gcc -Wall -o bin/daemon daemon/modules/*/* daemon/daemon.c   -pthread

console: console/console.c
	gcc -Wall -o bin/console console/console.c -pthread

clean:
	rm daemon console
