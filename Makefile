textversion : textversion.c
	gcc -o $@ $^
rushhour : rushhour.c
	gcc -o $@ $^ `pkg-config --cflags --libs gtk+-2.0`