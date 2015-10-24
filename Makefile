
SRC_DIR = cpp/
DATABASE = database/
BUILD_DIR = build/

OBJ_BASE = CNHmqtt.o INIReader.o ini.o CLogging.o
OBJS_BASE  := $(addprefix $(BUILD_DIR),$(OBJ_BASE))

OBJ_DBLIB = CNHDBAccess.o CDBValue.o
OBJS_DBLIB  := $(addprefix $(BUILD_DIR),$(OBJ_DBLIB))

CC_OUT = -o $(BUILD_DIR)$(notdir $@)

BIN_OUT = bin/

ALL_BIN = nh-test nh-irc GateKeeper nh-test-irc nh-irc-misc nh-irccat nh-monitor nh-matrix nh-temperature nh-vend nh-mail nh-tools nh-slack
ALL_BINS := $(addprefix $(BIN_OUT),$(ALL_BIN))

SLACK_INC=-I./SlackRtm/libwebsockets/lib -I./SlackRtm/include -I./SlackRtm/libwebsockets/build

CFLAGS = -Wall -Wextra -c -g
LFLAGS = -Wall -g
CC = g++


all: $(ALL_BINS) db/lib/CNHDBAccess.php

$(BIN_OUT)nh-test: $(BUILD_DIR)nh-test.o $(OBJS_BASE)
	g++ -lmosquitto -o $(BIN_OUT)nh-test $(BUILD_DIR)nh-test.o $(OBJS_BASE)

$(BIN_OUT)nh-vend: $(BUILD_DIR)nh-vend.o $(OBJS_BASE) $(OBJS_DBLIB)
	g++ -lmysqlclient -lmosquitto -lpthread -o $(BIN_OUT)nh-vend $(BUILD_DIR)nh-vend.o $(OBJS_BASE) $(OBJS_DBLIB)

$(BIN_OUT)nh-test-irc: $(BUILD_DIR)nh-test-irc.o $(BUILD_DIR)CNHmqtt_irc.o $(OBJS_BASE)
	g++ -lmosquitto -o $(BIN_OUT)nh-test-irc $(BUILD_DIR)nh-test-irc.o $(BUILD_DIR)CNHmqtt_irc.o $(OBJS_BASE)

$(BIN_OUT)nh-matrix: $(BUILD_DIR)nh-matrix.o $(BUILD_DIR)CNHmqtt_irc.o $(OBJS_BASE)
	g++ -lpthread -lmosquitto -o $(BIN_OUT)nh-matrix  $(BUILD_DIR)nh-matrix.o $(BUILD_DIR)CNHmqtt_irc.o $(OBJS_BASE)

$(BIN_OUT)nh-temperature: $(BUILD_DIR)nh-temperature.o $(OBJS_BASE) $(OBJS_DBLIB)
	g++ -lmosquitto -lmysqlclient -o $(BIN_OUT)nh-temperature $(BUILD_DIR)nh-temperature.o $(OBJS_BASE) $(OBJS_DBLIB)

$(BIN_OUT)nh-irc-misc: $(BUILD_DIR)nh-irc-misc.o $(BUILD_DIR)CNHmqtt_irc.o $(OBJS_BASE) $(OBJS_DBLIB)
	g++ -lmosquitto -lmysqlclient -o $(BIN_OUT)nh-irc-misc $(BUILD_DIR)nh-irc-misc.o $(BUILD_DIR)CNHmqtt_irc.o $(OBJS_BASE) $(OBJS_DBLIB)

$(BIN_OUT)nh-irccat: $(BUILD_DIR)nh-irccat.o $(BUILD_DIR)CNHmqtt_irc.o $(OBJS_BASE)
	g++ -lpthread -lmosquitto -o $(BIN_OUT)nh-irccat $(BUILD_DIR)nh-irccat.o $(BUILD_DIR)CNHmqtt_irc.o $(OBJS_BASE)

$(BIN_OUT)GateKeeper: $(BUILD_DIR)GateKeeper.o $(BUILD_DIR)CGatekeeper_door.o $(BUILD_DIR)CNHmqtt_irc.o $(OBJS_BASE) $(OBJS_DBLIB)
	g++ -lmysqlclient -lmosquitto -lrt -o $(BIN_OUT)GateKeeper $(BUILD_DIR)GateKeeper.o $(BUILD_DIR)CGatekeeper_door.o $(BUILD_DIR)CNHmqtt_irc.o $(OBJS_BASE) $(OBJS_DBLIB)

$(BIN_OUT)nh-tools: $(BUILD_DIR)nh-tools.o $(BUILD_DIR)nh-tools-bookings.o $(BUILD_DIR)CNHmqtt_irc.o $(OBJS_BASE) $(OBJS_DBLIB)
	g++ -lmysqlclient -lmosquitto -lrt -lpthread -lcurl -lical -ljson -luuid -o $(BIN_OUT)nh-tools $(BUILD_DIR)nh-tools.o $(BUILD_DIR)nh-tools-bookings.o $(BUILD_DIR)CNHmqtt_irc.o $(OBJS_BASE) $(OBJS_DBLIB)

$(BIN_OUT)nh-monitor: $(BUILD_DIR)nh-monitor.o $(OBJS_BASE) $(OBJS_DBLIB)
	g++ -lmysqlclient -lmosquitto -lpthread -o $(BIN_OUT)nh-monitor $(BUILD_DIR)nh-monitor.o $(OBJS_BASE) $(OBJS_DBLIB)

$(BIN_OUT)nh-irc: $(BUILD_DIR)nh-irc.o $(BUILD_DIR)irc.o $(OBJS_BASE)
	g++ -lmosquitto -lrt -lpthread -o $(BIN_OUT)nh-irc $(BUILD_DIR)nh-irc.o $(BUILD_DIR)irc.o $(OBJS_BASE)

SlackRtm/lib/libslackrtm.a: $(wildcard SlackRtm/cpp/*)
	cd SlackRtm ; make

$(BIN_OUT)nh-slack: $(BUILD_DIR)nh-slack.o $(BUILD_DIR)irc.o $(OBJS_BASE) SlackRtm/lib/libslackrtm.a
	g++ -lmosquitto -lrt -lpthread -lssl -lcrypto -lz -ljson -lcurl -o $(BIN_OUT)nh-slack $(BUILD_DIR)nh-slack.o $(BUILD_DIR)irc.o SlackRtm/lib/libslackrtm.a $(OBJS_BASE)

$(BIN_OUT)nh-mail: $(BUILD_DIR)nh-mail.o $(BUILD_DIR)CEmailProcess.o $(BUILD_DIR)INIReader.o $(BUILD_DIR)ini.o $(BUILD_DIR)CLogging.o $(OBJS_DBLIB)
	g++ -lmysqlclient -lmosquitto -o $(BIN_OUT)nh-mail $(BUILD_DIR)nh-mail.o $(BUILD_DIR)CEmailProcess.o $(BUILD_DIR)INIReader.o $(BUILD_DIR)ini.o $(BUILD_DIR)CLogging.o $(OBJS_DBLIB)

$(BIN_OUT)nh-macmon: $(BUILD_DIR)nh-macmon.o $(BUILD_DIR)CMacmon.o $(OBJS_BASE) $(OBJS_DBLIB)
	g++ -lpcap -lmysqlclient -lmosquitto -o $(BIN_OUT)nh-macmon $(BUILD_DIR)nh-macmon.o $(BUILD_DIR)CMacmon.o $(OBJS_BASE) $(OBJS_DBLIB)

web/nhweb/build/script/custom.js: $(wildcard web/nhweb/source/class/custom/*)
	sh nhweb.sh

nh-web: web/nhweb/build/script/custom.js

web2: nh-web db/lib/CNHDBAccess.php web/vend.php web/db.php web/wikiauth.php
	mkdir -p website/public/nhweb/
	mkdir -p website/public/wiki/
	cp db/lib/CNHDBAccess.php website/
	rsync -r --exclude=. web/nhweb/build/script website/public/nhweb/
	rsync -r --exclude=. web/nhweb/build/resource website/public/nhweb/
	cp web/nhweb/build/index.html website/public/nhweb/index.html
	rsync -r --exclude=. -r web/rpcservice website/public/
	rsync -r --exclude=. -r web/status website/public/
	rsync -r --exclude=. -r web/stats website/public/
	cp web/vend.php website/public/
	cp web/wikiauth.php website/public/wiki/
	cp web/db.php website/
	cp web/krb5_auth.php website/


$(BUILD_DIR)CNHmqtt.o: $(SRC_DIR)CNHmqtt.cpp $(SRC_DIR)CNHmqtt.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)CNHmqtt.cpp $(CC_OUT)

$(BUILD_DIR)nh-mail.o: $(SRC_DIR)nh-mail.cpp $(SRC_DIR)nh-mail.h db/lib/CNHDBAccess.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)nh-mail.cpp $(CC_OUT)

$(BUILD_DIR)CNHmqtt_irc.o: $(SRC_DIR)CNHmqtt_irc.cpp $(SRC_DIR)CNHmqtt_irc.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)CNHmqtt_irc.cpp $(CC_OUT)

$(BUILD_DIR)nh-test.o: $(SRC_DIR)nh-test.cpp $(SRC_DIR)nh-test.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)nh-test.cpp $(CC_OUT)

$(BUILD_DIR)nh-vend.o: $(SRC_DIR)nh-vend.cpp $(SRC_DIR)nh-vend.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)nh-vend.cpp $(CC_OUT)

$(BUILD_DIR)nh-test-irc.o: $(SRC_DIR)nh-test-irc.cpp $(SRC_DIR)nh-test-irc.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)nh-test-irc.cpp $(CC_OUT)

$(BUILD_DIR)nh-matrix.o: $(SRC_DIR)nh-matrix.cpp $(SRC_DIR)nh-matrix.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)nh-matrix.cpp $(CC_OUT)

$(BUILD_DIR)nh-temperature.o: $(SRC_DIR)nh-temperature.cpp $(SRC_DIR)nh-temperature.h db/lib/CNHDBAccess.o
	$(CC) $(CFLAGS) -c $(SRC_DIR)nh-temperature.cpp $(CC_OUT)

$(BUILD_DIR)nh-irc-misc.o: $(SRC_DIR)nh-irc-misc.cpp $(SRC_DIR)nh-irc-misc.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)nh-irc-misc.cpp $(CC_OUT)

$(BUILD_DIR)nh-macmon.o: $(SRC_DIR)nh-macmon.cpp $(SRC_DIR)nh-macmon.h db/lib/CNHDBAccess.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)nh-macmon.cpp $(CC_OUT)

$(BUILD_DIR)nh-irccat.o: $(SRC_DIR)nh-irccat.cpp $(SRC_DIR)nh-irccat.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)nh-irccat.cpp $(CC_OUT)

$(BUILD_DIR)GateKeeper.o: $(SRC_DIR)GateKeeper.cpp db/lib/CNHDBAccess.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)GateKeeper.cpp $(CC_OUT)

$(BUILD_DIR)CGatekeeper_door.o: $(SRC_DIR)CGatekeeper_door.cpp db/lib/CNHDBAccess.h $(SRC_DIR)CGatekeeper_door.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)CGatekeeper_door.cpp $(CC_OUT)

$(BUILD_DIR)nh-tools.o: $(SRC_DIR)nh-tools.cpp $(SRC_DIR)nh-tools.h db/lib/CNHDBAccess.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)nh-tools.cpp $(CC_OUT)

$(BUILD_DIR)nh-tools-bookings.o: $(SRC_DIR)nh-tools-bookings.cpp $(SRC_DIR)nh-tools-bookings.h db/lib/CNHDBAccess.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)nh-tools-bookings.cpp $(CC_OUT)

$(BUILD_DIR)nh-monitor.o: $(SRC_DIR)nh-monitor.cpp db/lib/CNHDBAccess.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)nh-monitor.cpp $(CC_OUT)

$(BUILD_DIR)nh-irc.o: $(SRC_DIR)nh-irc.cpp
	$(CC) $(CFLAGS) -c $(SRC_DIR)nh-irc.cpp $(CC_OUT)

$(BUILD_DIR)nh-slack.o: $(SRC_DIR)nh-slack.cpp SlackRtm/lib/libslackrtm.a
	$(CC) $(CFLAGS) $(SLACK_INC) -c $(SRC_DIR)nh-slack.cpp $(CC_OUT)

$(BUILD_DIR)irc.o: $(SRC_DIR)irc.cpp $(SRC_DIR)irc.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)irc.cpp  $(CC_OUT)

$(BUILD_DIR)ini.o: $(SRC_DIR)inireader/ini.c $(SRC_DIR)inireader/ini.h
	gcc -c $(SRC_DIR)inireader/ini.c $(CC_OUT)

$(BUILD_DIR)INIReaderTest.o: $(SRC_DIR)inireader/INIReaderTest.cpp
	$(CC) $(CFLAGS) -c $(SRC_DIR)inireader/INIReaderTest.cpp $(CC_OUT)

$(BUILD_DIR)INIReader.o: $(SRC_DIR)inireader/INIReader.cpp $(SRC_DIR)inireader/INIReader.h
	$(CC) -c $(SRC_DIR)inireader/INIReader.cpp $(CC_OUT)

INIReaderTest: $(BUILD_DIR)ini.o $(BUILD_DIR)INIReaderTest.o $(BUILD_DIR)INIReader.o
	g++ -o INIReaderTest $(BUILD_DIR)INIReader.o $(BUILD_DIR)INIReaderTest.o $(BUILD_DIR)ini.o

$(BUILD_DIR)CLogging.o: $(SRC_DIR)CLogging.cpp $(SRC_DIR)CLogging.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)CLogging.cpp $(CC_OUT)

$(BUILD_DIR)CEmailProcess.o: $(SRC_DIR)CEmailProcess.cpp $(SRC_DIR)CEmailProcess.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)CEmailProcess.cpp $(CC_OUT)

$(BUILD_DIR)CalcWordCount.o: $(SRC_DIR)CalcWordCount.cpp
	$(CC) $(CFLAGS) -c $(SRC_DIR)CalcWordCount.cpp $(CC_OUT)

$(BUILD_DIR)CMacmon.o: $(SRC_DIR)CMacmon.cpp $(SRC_DIR)CMacmon.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)CMacmon.cpp 


dblib: $(BUILD_DIR)gen_dblib

$(BUILD_DIR)gen_dblib: db/lib/gen_dblib.c
	gcc -Wall -o $(BUILD_DIR)gen_dblib db/lib/gen_dblib.c

db/lib/CNHDBAccess.cpp: $(BUILD_DIR)gen_dblib db/lib/CNHDBAccess_template.cpp $(wildcard db/sp/sp_*.sql)
	$(BUILD_DIR)gen_dblib db/lib $(wildcard db/sp/sp_*.sql)

db/lib/CNHDBAccess.h: $(BUILD_DIR)gen_dblib db/lib/CNHDBAccess_template.h $(wildcard db/sp/sp_*.sql)
	$(BUILD_DIR)gen_dblib db/lib $(wildcard db/sp/sp_*.sql)

db/lib/CNHDBAccess.php: $(BUILD_DIR)gen_dblib db/lib/CNHDBAccess_template.php $(wildcard db/sp/sp_*.sql)
	$(BUILD_DIR)gen_dblib db/lib $(wildcard db/sp/sp_*.sql)

$(BUILD_DIR)CNHDBAccess.o: db/lib/CNHDBAccess.cpp db/lib/CNHDBAccess.h 
	$(CC) $(CFLAGS) -c db/lib/CNHDBAccess.cpp -o $(BUILD_DIR)CNHDBAccess.o

$(BUILD_DIR)CDBValue.o: db/lib/CDBValue.cpp db/lib/CDBValue.cpp db/lib/CDBValue.cpp db/lib/CDBValue.h
	$(CC) $(CFLAGS) -c db/lib/CDBValue.cpp -o $(BUILD_DIR)CDBValue.o

clean:
	rm -fv build/*
	rm -fv bin/*
	rm -fv web/nhweb/build/script/custom.js
	rm -rfv website/
