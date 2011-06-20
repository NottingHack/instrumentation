all: nh-test INIReaderTest nh-irc nh-gk-if GateKeeper

install: nh-irc nh-gk-if GateKeeper
	stop GateKeeper
	stop nh-gk-if
	stop nh-holly
	cp nh-irc /home/instrumentation/bin/
	cp nh-gk-if /home/instrumentation/bin/
	cp GateKeeper /home/instrumentation/bin/
	chmod 555 /home/instrumentation/bin/nh-irc
	chmod 555 /home/instrumentation/bin/nh-gk-if
	chmod 555 /home/instrumentation/bin/GateKeeper
	start nh-holly
	start GateKeeper
	start nh-gk-if

nh-test: nh-test.o CNHmqtt.o INIReader.o ini.o CLogging.o
	g++ -lmosquitto -o nh-test nh-test.o CNHmqtt.o INIReader.o ini.o CLogging.o

GateKeeper: GateKeeper.o CNHmqtt.o INIReader.o ini.o GateKeeper_dbaccess.o CLogging.o
	g++ -lmysqlclient -lmosquitto -lrt -o GateKeeper GateKeeper.o CNHmqtt.o INIReader.o ini.o GateKeeper_dbaccess.o CLogging.o
	cp GateKeeper bin/

nh-gk-if: nh-gk-if.o CNHmqtt.o INIReader.o ini.o opendevice.o CLogging.o
	g++ -lmosquitto -lusb -lrt -o nh-gk-if nh-gk-if.o CNHmqtt.o INIReader.o ini.o opendevice.o CLogging.o
	cp nh-gk-if bin/ 

nh-irc: nh-irc.o CNHmqtt.o INIReader.o ini.o irc.o CLogging.o
	g++ -lmosquitto -lrt -o nh-irc nh-irc.o CNHmqtt.o INIReader.o ini.o irc.o CLogging.o
	cp nh-irc bin/

CNHmqtt.o: CNHmqtt.cpp CNHmqtt.h
	g++ -Wall -c CNHmqtt.cpp

nh-test.o: nh-test.cpp nh-test.h
	g++ -Wall -c nh-test.cpp

nh-gk-if.o: nh-gk-if.cpp  
	g++ -Wall -c nh-gk-if.cpp

GateKeeper.o: GateKeeper.cpp  
	g++ -Wall -c GateKeeper.cpp

GateKeeper_dbaccess.o: GateKeeper_dbaccess.cpp  
	g++ -Wall -c GateKeeper_dbaccess.cpp

opendevice.o: usb/opendevice.c
	g++ -Wall -c usb/opendevice.c

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
	rm -f GateKeeper_dbaccess.o GateKeeper.o GateKeeper mos_irc irc.o mos_irc.o nh-test.o CNHmqtt.o ini.o INIReader.o INIReaderTest.o nh-irc.o nh-gk-if.o opendevice.o CLogging.o

