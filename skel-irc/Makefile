all: nh-test-irc

nh-test-irc: nh-test-irc.o CNHmqtt_irc.o CNHmqtt.o INIReader.o ini.o CLogging.o
	g++ -lmosquitto -o nh-test-irc nh-test-irc.o CNHmqtt_irc.o CNHmqtt.o INIReader.o ini.o CLogging.o

CNHmqtt.o: CNHmqtt.cpp CNHmqtt.h
	g++ -Wall -c CNHmqtt.cpp

CNHmqtt_irc.o: CNHmqtt_irc.cpp CNHmqtt_irc.h
	g++ -Wall -c CNHmqtt_irc.cpp

nh-test-irc.o: nh-test-irc.cpp nh-test-irc.h
	g++ -Wall -c nh-test-irc.cpp

ini.o: inireader/ini.c inireader/ini.h
	gcc -c inireader/ini.c

INIReader.o: inireader/INIReader.cpp inireader/INIReader.h
	g++ -c inireader/INIReader.cpp

CLogging.o: CLogging.cpp CLogging.h
	g++ -Wall -c CLogging.cpp

clean:
	rm -f nh-test-irc nh-test-irc.o CNHmqtt_irc.o CNHmqtt.o ini.o INIReader.o n CLogging.o

