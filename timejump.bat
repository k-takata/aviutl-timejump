cl /LD /O1 timejump.cpp timejump.def /Fetimejump.auf /link /opt:nowin98 kernel32.lib user32.lib gdi32.lib /merge:.rdata=.text
