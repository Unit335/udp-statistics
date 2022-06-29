udp-statistics: udp-statshow udp-stat

udp-statshow: udp-statshow.c
	gcc -o udp-statshow udp-statshow.c -lrt
		
udp-stat: udpstat.c udpstat.h
	gcc -pthread -o udp-stat udpstat.c -lrt
