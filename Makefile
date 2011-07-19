all: nh-test INIReaderTest nh-irc GateKeeper nh-test-irc nh-irccat

install: install_nh_holly install_gatekeeper

install_gatekeeper: GateKeeper
	stop GateKeeper
	cp GateKeeper /home/instrumentation/bin/
	chmod 555 /home/instrumentation/bin/GateKeeper
	start GateKeeper

install_nh_holly: nh-irc
	stop nh-holly
	cp nh-irc /home/instrumentation/bin/
	chmod 555 /home/instrumentation/bin/nh-irc
	start nh-holly

nh-test: nh-test.o CNHmqtt.o INIReader.o ini.o CLogging.o
	g++ -lmosquitto -o nh-test nh-test.o CNHmqtt.o INIReader.o ini.o CLogging.o

nh-test-irc: nh-test-irc.o CNHmqtt_irc.o CNHmqtt.o INIReader.o ini.o CLogging.o
	g++ -lmosquitto -o nh-test-irc nh-test-irc.o CNHmqtt_irc.o CNHmqtt.o INIReader.o ini.o CLogging.o

nh-irccat: nh-irccat.o CNHmqtt_irc.o CNHmqtt.o INIReader.o ini.o CLogging.o
	g++ -lpthread -lmosquitto -o nh-irccat nh-irccat.o CNHmqtt_irc.o CNHmqtt.o INIReader.o ini.o CLogging.o

GateKeeper: GateKeeper.o CNHmqtt.o CNHmqtt_irc.o INIReader.o ini.o GateKeeper_dbaccess.o CLogging.o
	g++ -lmysqlclient -lmosquitto -lrt -o GateKeeper GateKeeper.o CNHmqtt.o CNHmqtt_irc.o INIReader.o ini.o GateKeeper_dbaccess.o CLogging.o
	cp GateKeeper bin/

nh-irc: nh-irc.o CNHmqtt.o INIReader.o ini.o irc.o CLogging.o
	g++ -lmosquitto -lrt -o nh-irc nh-irc.o CNHmqtt.o INIReader.o ini.o irc.o CLogging.o
	cp nh-irc bin/

CNHmqtt.o: CNHmqtt.cpp CNHmqtt.h
	g++ -Wall -c CNHmqtt.cpp

CNHmqtt_irc.o: CNHmqtt_irc.cpp CNHmqtt_irc.h
	g++ -Wall -c CNHmqtt_irc.cpp

nh-test.o: nh-test.cpp nh-test.h
	g++ -Wall -c nh-test.cpp

nh-test-irc.o: nh-test-irc.cpp nh-test-irc.h
	g++ -Wall -c nh-test-irc.cpp

irccat.o: nh-irccat.cpp nh-irccat.h
	g++ -Wall -c nh-irccat.cpp

GateKeeper.o: GateKeeper.cpp  
	g++ -Wall -c GateKeeper.cpp

GateKeeper_dbaccess.o: GateKeeper_dbaccess.cpp  
	g++ -Wall -c GateKeeper_dbaccess.cpp

nh-irc.o: nh-irc.cpp
	g++ -Wall -c nh-irc.cpp

irc.o: irc.cpp irc.h
	g++ -c irc.cpp 

ini.o: inireader/ini.c inireader/ini.h
	gcc -c inireader/ini.c

INIReaderTest.o: inireader/INIReaderTest.cpp
	g++ -c inireader/INIReaderTest.cpp

INIReader.o: inireader/INIReader.cpp inireader/INIReader.h
	g++ -c inireader/INIReader.cpp

INIReaderTest: ini.o INIReaderTest.o INIReader.o
	g++ -o INIReaderTest INIReader.o INIReaderTest.o ini.o

CLogging.o: CLogging.cpp CLogging.h
	g++ -c CLogging.cpp

clean:
	rm -f CNHmqtt_irc.o nh-irc nh-test nh-irccat nh-test-irc INIReaderTest GateKeeper_dbaccess.o GateKeeper.o GateKeeper mos_irc irc.o mos_irc.o nh-test.o CNHmqtt.o ini.o INIReader.o INIReaderTest.o nh-irc.o nh-gk-if.o CLogging.o

