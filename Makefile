all: nh-test INIReaderTest nh-irc GateKeeper nh-test-irc nh-irc-misc nh-irccat nh-monitor nh-matrix nh-temperature nh-vend

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

nh-vend: nh-vend.o CNHmqtt.o INIReader.o ini.o CLogging.o db/lib/CNHDBAccess.o
	g++ -lmysqlclient -lmosquitto -lpthread -o nh-vend nh-vend.o CNHmqtt.o INIReader.o ini.o CLogging.o db/lib/CNHDBAccess.o

nh-test-irc: nh-test-irc.o CNHmqtt_irc.o CNHmqtt.o INIReader.o ini.o CLogging.o
	g++ -lmosquitto -o nh-test-irc nh-test-irc.o CNHmqtt_irc.o CNHmqtt.o INIReader.o ini.o CLogging.o

nh-matrix: nh-matrix.o CNHmqtt_irc.o CNHmqtt.o INIReader.o ini.o CLogging.o
	g++ -lmosquitto -o nh-matrix nh-matrix.o CNHmqtt_irc.o CNHmqtt.o INIReader.o ini.o CLogging.o

nh-temperature: nh-temperature.o CNHmqtt.o INIReader.o ini.o CLogging.o db/lib/CNHDBAccess.o
	g++ -lmosquitto -lmysqlclient -o nh-temperature nh-temperature.o CNHmqtt.o INIReader.o ini.o CLogging.o db/lib/CNHDBAccess.o

nh-irc-misc: nh-irc-misc.o CNHmqtt_irc.o CNHmqtt.o INIReader.o ini.o CLogging.o db/lib/CNHDBAccess.o
	g++ -lmosquitto -lmysqlclient -o nh-irc-misc nh-irc-misc.o CNHmqtt_irc.o CNHmqtt.o INIReader.o ini.o CLogging.o db/lib/CNHDBAccess.o

nh-irccat: nh-irccat.o CNHmqtt_irc.o CNHmqtt.o INIReader.o ini.o CLogging.o
	g++ -lpthread -lmosquitto -o nh-irccat nh-irccat.o CNHmqtt_irc.o CNHmqtt.o INIReader.o ini.o CLogging.o

GateKeeper: GateKeeper.o CNHmqtt.o CNHmqtt_irc.o INIReader.o ini.o db/lib/CNHDBAccess.o CLogging.o
	g++ -lmysqlclient -lmosquitto -lrt -o GateKeeper GateKeeper.o CNHmqtt.o CNHmqtt_irc.o INIReader.o ini.o db/lib/CNHDBAccess.o CLogging.o
	cp GateKeeper bin/

nh-monitor: nh-monitor.o CNHmqtt.o INIReader.o ini.o db/lib/CNHDBAccess.o CLogging.o
	g++ -lmysqlclient -lmosquitto -lpthread -o nh-monitor nh-monitor.o CNHmqtt.o INIReader.o ini.o db/lib/CNHDBAccess.o CLogging.o
	cp nh-monitor bin/

nh-irc: nh-irc.o CNHmqtt.o INIReader.o ini.o irc.o CLogging.o
	g++ -lmosquitto -lrt -o nh-irc nh-irc.o CNHmqtt.o INIReader.o ini.o irc.o CLogging.o
	cp nh-irc bin/

CNHmqtt.o: CNHmqtt.cpp CNHmqtt.h
	g++ -Wall -c CNHmqtt.cpp

CNHmqtt_irc.o: CNHmqtt_irc.cpp CNHmqtt_irc.h
	g++ -Wall -c CNHmqtt_irc.cpp

nh-test.o: nh-test.cpp nh-test.h
	g++ -Wall -c nh-test.cpp

nh-vend.o: nh-vend.cpp nh-vend.h
	g++ -Wall -c nh-vend.cpp

nh-test-irc.o: nh-test-irc.cpp nh-test-irc.h
	g++ -Wall -c nh-test-irc.cpp

nh-matrix.o: nh-matrix.cpp nh-matrix.h
	g++ -Wall -c nh-matrix.cpp

nh-temperature.o: nh-temperature.cpp nh-temperature.h db/lib/CNHDBAccess.o
	g++ -Wall -c nh-temperature.cpp

nh-irc-misc.o: nh-irc-misc.cpp nh-irc-misc.h
	g++ -Wall -c nh-irc-misc.cpp

irccat.o: nh-irccat.cpp nh-irccat.h
	g++ -Wall -c nh-irccat.cpp

GateKeeper.o: GateKeeper.cpp db/lib/CNHDBAccess.o
	g++ -Wall -c GateKeeper.cpp

nh-monitor.o: nh-monitor.cpp db/lib/CNHDBAccess.o
	g++ -Wall -c nh-monitor.cpp

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

db/lib/gen_dblib: db/lib/gen_dblib.c
	gcc -Wall -o db/lib/gen_dblib db/lib/gen_dblib.c

db/lib/CNHDBAccess.cpp: db/lib/gen_dblib db/lib/CNHDBAccess_template.cpp $(wildcard db/sp_*.sql)
	db/lib/gen_dblib db/lib $(wildcard db/sp_*.sql)

db/lib/CNHDBAccess.h: db/lib/gen_dblib db/lib/CNHDBAccess_template.h $(wildcard db/sp_*.sql)
	db/lib/gen_dblib db/lib $(wildcard db/sp_*.sql)

db/lib/CNHDBAccess.o: db/lib/CNHDBAccess.cpp db/lib/CNHDBAccess.h 
	g++ -Wall -c db/lib/CNHDBAccess.cpp -o db/lib/CNHDBAccess.o



clean:
	rm -f db/lib/gen_dblib db/lib/CNHDBAccess.cpp db/lib/CNHDBAccess.h db/lib/CNHDBAccess.o nh-monitor.o nh-monitor CNHDBAccess.o nh-irc-misc.o nh-irccat.o nh-test-irc.o CNHmqtt_irc.o nh-irc nh-test nh-irccat nh-test-irc nh-irc-misc INIReaderTest GateKeeper_dbaccess.o GateKeeper.o GateKeeper mos_irc irc.o mos_irc.o nh-test.o CNHmqtt.o ini.o INIReader.o INIReaderTest.o nh-irc.o nh-gk-if.o CLogging.o nh-matrix.o nh-matrix nh-temperature.o nh-temperature nh-vend.o nh-vend

