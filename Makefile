program = "udp-statistics_0.4.0_amd64"
ROOT_DIR:=$(shell dirname $(realpath $(firstword $(MAKEFILE_LIST))))

udp-statistics: udp-statshow udp-stat

udp-statshow: udp-statshow.c
	gcc -o udp-statshow udp-statshow.c -lrt
		
udp-stat: udpstat.c udpstat.h
	gcc -pthread -o udp-stat udpstat.c -lrt
	

deb: udp.deb clean
	
udp.deb:
	mkdir -p $(program)/usr/local/bin
	mkdir $(program)/DEBIAN
	cp $(ROOT_DIR)/DEBIAN/control $(ROOT_DIR)/$(program)/DEBIAN/
	cp $(ROOT_DIR)/udp-stat $(ROOT_DIR)/$(program)/usr/local/bin/
	cp $(ROOT_DIR)/udp-statshow $(ROOT_DIR)/$(program)/usr/local/bin/
	dpkg-deb --build --root-owner-group $(program)
	
clean:
	rm -r $(program)
