###
 # CS5700 Fall 2019
 # Project 5: Roll Your Own CDN
 # @author Azamat Sarkytbayev
 # NU ID: 001873077
 ##

 # target: dependencies
 #	action

CXX = g++
CXXFLAGS = -std=c++11

all: deployCDN runCDN stopCDN dnsserver httpserver

CDN: deployCDN runCDN stopCDN

deployCDN:
	chmod +x deployCDN

runCDN:
	chmod +x runCDN

stopCDN:
	chmod +x stopCDN

httpserver:
	chmod +x httpserver

dnsserver: main.o dnsserver.o
	$(CXX) $(CXXFLAGS) dnsserver.o main.o -o dnsserver

main.o: main.cpp
	$(CXX) $(CXXFLAGS) -c main.cpp

dnsserver.o: dnsserver.cpp dnsserver.h
	$(CXX) $(CXXFLAGS) -c dnsserver.cpp

clean:
	rm *.o dnsserver a.out *.txt