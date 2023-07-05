SHARED := -shared -fPIC
SOURCE_DIR := ../demo/
INCS := -I ../include/ -I ../include/redis/ -I ../demo/

target : libnav.so libpublisher.so librbmcu.so

libnav.so : $(SOURCE_DIR)nav.c $(SOURCE_DIR)pid.c
	gcc -g3 $(SOURCE_DIR)/nav.c $(SOURCE_DIR)pid.c -o libnav.so $(INCS) $(SHARED)

libpublisher.so : $(SOURCE_DIR)publisher.c
	gcc -g3 $(SOURCE_DIR)/publisher.c -o libpublisher.so $(INCS) $(SHARED)

librbmcu.so : $(SOURCE_DIR)rb_mcu.c
	gcc -g3 $(SOURCE_DIR)/rb_mcu.c -o librbmcu.so $(INCS) $(SHARED)

all : $(target)

clean :
	rm -f libnav.so libpublisher.so librbmcu.so

PHONY : clean all
