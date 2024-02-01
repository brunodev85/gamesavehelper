OBJS=obj/main.o obj/common_gamesave_wnd.o obj/resource.o
INCLUDE_DIR=-I.\include
WARNS=-Wall

LDFLAGS=-s -lcomctl32 -Wl,--subsystem,windows
RC=windres
CFLAGS=-O3 -std=c99 -DUNICODE -D_UNICODE -D_WIN32_IE=0x0500 -DWINVER=0x500 ${WARNS}

all: GamesaveHelper.exe

GamesaveHelper.exe: ${OBJS}
	${CC} -o "$@" ${OBJS} ${LDFLAGS}

clean:
	del obj\*.o "GamesaveHelper.exe"

obj:
	mkdir obj

obj/%.o: src/%.c obj
	${CC} ${CFLAGS} ${INCLUDE_DIR} -c $< -o $@

obj/resource.o: res/resource.rc res/Application.manifest res/main.ico res/backup.ico res/restore.ico include/resource.h
	${RC} ${INCLUDE_DIR} -I.\res -i $< -o $@