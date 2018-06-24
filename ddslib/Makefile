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
SOURCES=$(sort $(shell find -L lib1 lib2 src -name '*.c'))
HEADERS=$(sort $(shell find -L      lib2 src -name '*.h'))
DEPENDS=$(patsubst %.c, dep/%.d, $(SOURCES))
OBJECTS=$(patsubst %.c, obj/%.o, $(SOURCES))

libddsmgr.a : $(OBJECTS)
	@ar crs $@ $^

dep/%.d : %.c
	@mkdir -p $$(dirname $@)
	@gcc $(INCLUDES) -MF $@ -MM -MT $(patsubst %.c, obj/%.o, $<) $<

obj/%.o : %.c
	@mkdir -p $$(dirname $@)
	@gcc $(INCLUDES) -O2 -Wall -g -o $@ -c $<

-include $(DEPENDS)

