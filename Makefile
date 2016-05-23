

tcc-loader.so: tcc-loader.c
	gcc -Wall -DPURPLE_PLUGINS tcc-loader.c -shared -o tcc-loader.so `pkg-config --libs --cflags purple glib-2.0` -I/usr/include/ -ltcc
	
install: tcc-loader.so
	cp tcc-loader.so `pkg-config --variable=plugindir purple`
