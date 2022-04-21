all: histclient_th histclient histserver histserver_th
histclient_th: histclient_th.c
	gcc -Wall -o histclient_th histclient_th.c -lrt -pthread
histserver_th: histserver_th.c
	gcc -Wall -o histserver_th histserver_th.c -lrt -pthread
histclient: histclient.c
	gcc -Wall -o histclient histclient.c -lrt
histserver: histserver.c
	gcc -Wall -o histserver histserver.c -lrt
clean:
	rm -f histserver_th histclient_th histserver histclient