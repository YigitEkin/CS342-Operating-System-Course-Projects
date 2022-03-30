histserver_th: histserver_th.c
	gcc -g -pthread histserver_th.c -o histserver_th
histclient_th: histclient_th.o
	gcc -o histclient_th histclient_th.o 
histclient_th.o: histclient_th.c
	gcc -c histclient_th.c
clean:
	rm -f histserver_th histclient_th a.out