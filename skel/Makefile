all: nh-test

nh-test: nh-test.o CNHmqtt.o INIReader.o ini.o CLogging.o
	g++ -lmosquitto -o nh-test nh-test.o CNHmqtt.o INIReader.o ini.o CLogging.o

CNHmqtt.o: CNHmqtt.cpp CNHmqtt.h
	g++ -Wall -c CNHmqtt.cpp

nh-test.o: nh-test.cpp nh-test.h
	g++ -Wall -c nh-test.cpp

ini.o: inireader/ini.c inireader/ini.h
	gcc -c inireader/ini.c

INIReader.o: inireader/INIReader.cpp inireader/INIReader.h
	g++ -c inireader/INIReader.cpp

CLogging.o: CLogging.cpp CLogging.h
	g++ -Wall -c CLogging.cpp

clean:
	rm -f nh-test nh-test.o CNHmqtt.o ini.o INIReader.o n CLogging.o

