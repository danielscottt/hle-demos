default:
	$(MAKE) avg
	$(MAKE) cthreads
avg:
	mkdir -p build
	gcc -g --std=gnu99 -c -I include -o build/liblocker.so src/locker.c
	gcc -g --std=gnu99 -I include -L build/ -llocker -lpthread -o build/avg src/main.c
cthreads:
	mkdir -p build
	gcc -g --std=gnu99 -c -I include -o build/liblocker.so src/locker.c
	gcc -g --std=gnu99 -I include -L build/ -llocker -lpthread -o build/cthreads src/mainv2.c
