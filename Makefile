all: nh-test INIReaderTest nh-irc GateKeeper nh-test-irc nh-irc-misc nh-irccat nh-monitor nh-matrix nh-temperature nh-vend nh-mini-matrix nh-mail db/lib/CNHDBAccess.php
 
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

install_irccat: nh-irccat
	stop nh-irccat
	cp nh-irccat /home/instrumentation/bin/
	chmod 555 /home/instrumentation/nh-irccar
	cp command_runner.py /home/instrumentation/bin/
	chmod 555 /home/instrumentaion/bin/command_runner.py
	cp -r irccat /home/instrumentation/bin/
	chmod 755 /home/instrumentaion/bin/irccat
	chmod 555 /home/instrumentaion/bin/irccat/*
	start nh-irccat
	
install_upstart:
	cp upstart/* /etc/init/
	
install_conf:
	cp -r conf /home/instrumentaion/

nh-test: nh-test.o CNHmqtt.o INIReader.o ini.o CLogging.o
	g++ -lmosquitto -o nh-test nh-test.o CNHmqtt.o INIReader.o ini.o CLogging.o

nh-vend: nh-vend.o CNHmqtt.o INIReader.o ini.o CLogging.o db/lib/CNHDBAccess.o
	g++ -lmysqlclient -lmosquitto -lpthread -o nh-vend nh-vend.o CNHmqtt.o INIReader.o ini.o CLogging.o db/lib/CNHDBAccess.o
	cp nh-vend bin/

nh-test-irc: nh-test-irc.o CNHmqtt_irc.o CNHmqtt.o INIReader.o ini.o CLogging.o
	g++ -lmosquitto -o nh-test-irc nh-test-irc.o CNHmqtt_irc.o CNHmqtt.o INIReader.o ini.o CLogging.o

nh-mini-matrix: nh-mini-matrix.o CNHmqtt_irc.o CNHmqtt.o INIReader.o ini.o CLogging.o
	g++ -lmosquitto -lpthread -o nh-mini-matrix nh-mini-matrix.o CNHmqtt_irc.o CNHmqtt.o INIReader.o ini.o CLogging.o
	cp nh-mini-matrix bin/

#nh-udp2mqtt: nh-udp2mqtt.o CNHmqtt.o INIReader.o ini.o CLogging.o
#	g++ -lmosquitto -lpthread -o nh-udp2mqtt nh-udp2mqtt.o CNHmqtt.o INIReader.o ini.o CLogging.o
#	cp nh-udp2mqtt bin/

nh-matrix: nh-matrix.o CNHmqtt_irc.o CNHmqtt.o INIReader.o ini.o CLogging.o
	g++ -lmosquitto -o nh-matrix nh-matrix.o CNHmqtt_irc.o CNHmqtt.o INIReader.o ini.o CLogging.o
	cp nh-matrix bin/

nh-temperature: nh-temperature.o CNHmqtt.o INIReader.o ini.o CLogging.o db/lib/CNHDBAccess.o
	g++ -lmosquitto -lmysqlclient -o nh-temperature nh-temperature.o CNHmqtt.o INIReader.o ini.o CLogging.o db/lib/CNHDBAccess.o
	cp nh-temperature bin/

nh-irc-misc: nh-irc-misc.o CNHmqtt_irc.o CNHmqtt.o INIReader.o ini.o CLogging.o db/lib/CNHDBAccess.o
	g++ -lmosquitto -lmysqlclient -o nh-irc-misc nh-irc-misc.o CNHmqtt_irc.o CNHmqtt.o INIReader.o ini.o CLogging.o db/lib/CNHDBAccess.o
	cp nh-irc-misc bin/

nh-irccat: nh-irccat.o CNHmqtt_irc.o CNHmqtt.o INIReader.o ini.o CLogging.o
	g++ -lpthread -lmosquitto -o nh-irccat nh-irccat.o CNHmqtt_irc.o CNHmqtt.o INIReader.o ini.o CLogging.o
	cp nh-irccat bin/

GateKeeper: GateKeeper.o CNHmqtt.o CNHmqtt_irc.o INIReader.o ini.o db/lib/CNHDBAccess.o CLogging.o
	g++ -lmysqlclient -lmosquitto -lrt -o GateKeeper GateKeeper.o CNHmqtt.o CNHmqtt_irc.o INIReader.o ini.o db/lib/CNHDBAccess.o CLogging.o
	cp GateKeeper bin/

nh-monitor: nh-monitor.o CNHmqtt.o INIReader.o ini.o db/lib/CNHDBAccess.o CLogging.o
	g++ -lmysqlclient -lmosquitto -lpthread -o nh-monitor nh-monitor.o CNHmqtt.o INIReader.o ini.o db/lib/CNHDBAccess.o CLogging.o
	cp nh-monitor bin/

nh-irc: nh-irc.o CNHmqtt.o INIReader.o ini.o irc.o CLogging.o
	g++ -lmosquitto -lrt -o nh-irc nh-irc.o CNHmqtt.o INIReader.o ini.o irc.o CLogging.o
	cp nh-irc bin/

nh-mail: nh-mail.o INIReader.o ini.o CLogging.o db/lib/CNHDBAccess.o CEmailProcess.o
	g++ -lmysqlclient -lmosquitto -o nh-mail nh-mail.o INIReader.o ini.o CLogging.o db/lib/CNHDBAccess.o CEmailProcess.o
	cp nh-mail bin/

web/nhweb/build/script/custom.js: $(wildcard web/nhweb/source/class/custom/*)
	sh nhweb.sh

nh-web: web/nhweb/build/script/custom.js

web2: nh-web db/lib/CNHDBAccess.php web/pwreset.php web/vend.php web/db.php
	mkdir -p website/public/nhweb/
	cp db/lib/CNHDBAccess.php website/
	rsync -r --exclude=. web/nhweb/build/script website/public/nhweb/
	rsync -r --exclude=. web/nhweb/build/resource website/public/nhweb/
	cp web/nhweb/build/index.html website/public/nhweb/index.html
	rsync -r --exclude=. -r web/rpcservice website/public/
	rsync -r --exclude=. -r web/status website/public/
	cp web/pwreset.php website/public/
	cp web/vend.php website/public/
#	cp web/wikiauth.php website/public/
	cp web/db.php website/
	cp web/krb5_auth.php website/


CNHmqtt.o: CNHmqtt.cpp CNHmqtt.h
	g++ -Wall -c CNHmqtt.cpp

nh-mail.o: nh-mail.cpp nh-mail.h
	g++ -Wall -c nh-mail.cpp

CNHmqtt_irc.o: CNHmqtt_irc.cpp CNHmqtt_irc.h
	g++ -Wall -c CNHmqtt_irc.cpp

nh-test.o: nh-test.cpp nh-test.h
	g++ -Wall -c nh-test.cpp

nh-vend.o: nh-vend.cpp nh-vend.h
	g++ -Wall -c nh-vend.cpp

nh-test-irc.o: nh-test-irc.cpp nh-test-irc.h
	g++ -Wall -c nh-test-irc.cpp

nh-mini-matrix.o: nh-mini-matrix.cpp nh-mini-matrix.h
	g++ -Wall -c nh-mini-matrix.cpp

#nh-udp2mqtt.o: nh-udp2mqtt.cpp nh-udp2mqtt.h
#	g++ -Wall -c nh-udp2mqtt.cpp

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
	g++ -Wall -Wextra -c nh-irc.cpp

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

CEmailProcess.o: CEmailProcess.cpp CEmailProcess.h
	g++ -Wall -c CEmailProcess.cpp

dblib: db/lib/gen_dblib

db/lib/gen_dblib: db/lib/gen_dblib.c
	gcc -Wall -o db/lib/gen_dblib db/lib/gen_dblib.c

db/lib/CNHDBAccess.cpp: db/lib/gen_dblib db/lib/CNHDBAccess_template.cpp $(wildcard db/sp_*.sql)
	db/lib/gen_dblib db/lib $(wildcard db/sp_*.sql)

db/lib/CNHDBAccess.h: db/lib/gen_dblib db/lib/CNHDBAccess_template.h $(wildcard db/sp_*.sql)
	db/lib/gen_dblib db/lib $(wildcard db/sp_*.sql)

db/lib/CNHDBAccess.php: db/lib/gen_dblib db/lib/CNHDBAccess_template.php $(wildcard db/sp_*.sql)
	db/lib/gen_dblib db/lib $(wildcard db/sp_*.sql)

db/lib/CNHDBAccess.o: db/lib/CNHDBAccess.cpp db/lib/CNHDBAccess.h 
	g++ -Wall -c db/lib/CNHDBAccess.cpp -o db/lib/CNHDBAccess.o

clean:
	rm -f db/lib/gen_dblib db/lib/CNHDBAccess.cpp db/lib/CNHDBAccess.h db/lib/CNHDBAccess.o nh-monitor.o nh-monitor CNHDBAccess.o nh-irc-misc.o nh-irccat.o nh-test-irc.o CNHmqtt_irc.o nh-irc nh-test nh-irccat nh-test-irc nh-irc-misc INIReaderTest GateKeeper_dbaccess.o GateKeeper.o GateKeeper mos_irc irc.o mos_irc.o nh-test.o CNHmqtt.o ini.o INIReader.o INIReaderTest.o nh-irc.o nh-gk-if.o CLogging.o nh-matrix.o nh-matrix nh-temperature.o nh-temperature nh-vend.o nh-vend nh-mini-matrix.o nh-mini-matrix nh-mail nh-mail.o CEmailProcess.o web/nhweb/build/script/custom.js
	rm -rf website/
