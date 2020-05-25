LIBS = kernel32.lib user32.lib gdi32.lib

all: timejump.auf

timejump.auf: timejump.obj timejump.def
	$(CC) /LD /Fe$@ timejump.obj timejump.def $(LIBS) /link /merge:.rdata=.text

timejump.obj: timejump.cpp
	$(CC) /c /O1 $*.cpp
