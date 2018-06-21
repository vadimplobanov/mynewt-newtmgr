MYNEWT_PROJ=~/dds

SETUP_OUTPUT:=$(shell ./setup.sh $(MYNEWT_PROJ))
ifneq ($(.SHELLSTATUS), 0)
 $(error $(SETUP_OUTPUT))
endif

INCLUDE_DIRS=lib1/dds_c/infrastructure \
             lib1/dds_c/publication    \
             lib1/dds_c/subscription   \
             lib1/dds_c/topic          \
             lib1/dds_c/type           \
             lib1/ext

INCLUDES=$(foreach i, $(INCLUDE_DIRS), -I $i)
SOURCES=$(sort $(shell find -L * -name '*.c'))
OBJECTS=$(patsubst %.c, obj/%.o, $(SOURCES))

libddsmgr.a : $(OBJECTS)
	@ar crs $@ $^

obj/%.o : %.c
	@mkdir -p $$(dirname $@)
	@gcc $(INCLUDES) -O2 -Wall -o $@ -c $^

