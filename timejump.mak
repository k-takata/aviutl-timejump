CPPFLAGS = /O1 /W4
LIBS = kernel32.lib user32.lib gdi32.lib
LDFLAGS = /merge:.rdata=.text

all: timejump.auf

timejump.auf: timejump.obj timejump.def
	$(CC) /LD /Fe$@ timejump.obj timejump.def $(LIBS) /link $(LDFLAGS)

timejump.obj: timejump.cpp
