_libdir:=       $(abspath $(dir $(lastword ${MAKEFILE_LIST})))

CPPFLAGS+=	-DF_CPU=48000000
ifdef XTAL
CPPFLAGS+=	-DEXTERNAL_XTAL=$(XTAL)
endif
ifdef DEVICE
CPPFLAGS+=	-DTARGET_DEVICE=$(DEVICE)
endif

-include .mchckrc
-include ${_libdir}/../.mchckrc

ifndef MAKECMDGOALS
.DEFAULT_GOAL:=
endif

is-make-clean=	$(filter clean realclean,${MAKECMDGOALS})


define _include_libs
_libdir-$(1):=	$$(addprefix $${_libdir}/lib/,$(1))
_forceobjs-$(1)=	$$(addsuffix .o, $$(basename $$(addprefix $(1)-lib-,$${SRCS.force-$(1)})))
FORCEOBJS+=	$${_forceobjs-$(1)}
_objs-$(1)=	$$(addsuffix .o, $$(basename $$(addprefix $(1)-lib-,$${SRCS-$(1)}))) $${_forceobjs-$(1)}
_objs-$(1)+=	$$(addsuffix .o, $$(basename $$(addprefix $(1)-lib-,$${CXXSRCS-$(1)}))) $${_forceobjs-$(1)}
_allobjs+=	$${_objs-$(1)}
_libobjs+=	$${_objs-$(1)}
CLEANFILES+=	$${_objs-$(1)}
_deps-$(1)=	$$(addsuffix .d, $$(basename $$(addprefix $(1)-lib-,$${SRCS-$(1)} $${SRCS.force-$(1)})))
_deps-$(1)+=	$$(addsuffix .d, $$(basename $$(addprefix $(1)-lib-,$${CXXSRCS-$(1)} $${CXXSRCS.force-$(1)})))
DEPS+=	$${_deps-$(1)}

$${LIBDEPCACHE}/$(1)-lib-%.o $(1)-lib-%.o: $${_libdir-$(1)}/%.c
	$$(COMPILE.c) $$(OUTPUT_OPTION) $$<
$(1)-lib-%.d: $${_libdir-$(1)}/%.c
	$$(GENERATE.d)
endef

GENERATE.d=	$(CC) -MM ${CPPFLAGS} -MT $@ -MT ${@:.d=.o} -MP -MF $@ $<


# Common config

CPPFLAGS+=	-I${_libdir}/include -I${_libdir}/lib
CPPFLAGS+=	-std=gnu11
CFLAGS+=	-fplan9-extensions
CFLAGS+=	-ggdb3
ifndef DEBUG
CFLAGS+=	${COPTFLAGS}
else
NO_LTO=		no-lto
endif
CFLAGS+=	${CWARNFLAGS}

ifeq (${SRCS},)
ifeq (${CXXSRCS},)
SRCS?=	${PROG}.c
_no_src_given:=	true
else
CXXSRCS?=	${PROG}.cpp
_no_src_given:=	true
endif
endif

ifdef DEVICE
CPPFLAGS+=	-DTARGET_DEVICE=$(DEVICE)
endif

SRCS +=toolchain/udad/udad.c toolchain/udad/systick.c toolchain/udad/i2s.c toolchain/udad/codec.c toolchain/udad/softi2c.c toolchain/udad/serial.c toolchain/udad/serial1.c toolchain/udad/audio.c toolchain/udad/quadrature.c

_compilesrc=	$(filter %.c,${SRCS}) $(filter %.cpp,${CXXSRCS})
_objs=	$(addsuffix .o, $(basename ${_compilesrc} ${_gensrc}))
CLEANFILES+=	${_objs}
OBJS+=	${_objs}
_allobjs+=	${OBJS}
DEPS+=	$(addsuffix .d, $(basename ${_compilesrc} ${_gensrc}))
CLEANFILES+=	${DEPS}


# This should be in linkdep.mk, but it is needed before, in the
# expansion of _include_libs.
LIBDEPCACHE=	${_libdir}/cache


# Host config (VUSB)

ifeq (${TARGET},host)
CPPFLAGS+=	-DTARGET_HOST
CFLAGS+=	-fshort-enums

all: ${PROG}

include ${_libdir}/lib/Makefile.part
$(foreach _uselib,${SRCS.libs},$(eval $(call _include_libs,$(_uselib))))

${PROG}: ${_allobjs}
	$(LINK.c) $^ ${LDLIBS} -o $@

CLEANFILES+=	${PROG}
else

# MCHCK config



CC=	arm-none-eabi-gcc
CXX=	arm-none-eabi-g++
LD=	arm-none-eabi-ld
AR=	arm-none-eabi-ar
AS=	arm-none-eabi-as
OBJCOPY=	arm-none-eabi-objcopy
GDB=	arm-none-eabi-gdb
DFUUTIL?=	dfu-util
RUBY?=	ruby

export PATH:=	$(shell cd $(TOOLS) && pwd):${PATH}

ifeq ($(shell which $(CC) 2>/dev/null),)
SATDIR?=	$(HOME)/sat
endif
ifdef SATDIR
PATH:=	${SATDIR}/bin:${PATH}
endif
export PATH

COMPILER_PATH=	${_libdir}/scripts
export COMPILER_PATH

TARGET?=	MK20DX32VFM5
DFUVID?=	2323
DFUPID?=	0001

include ${_libdir}/${TARGET}.mk

COPTFLAGS?=	-Os
CWARNFLAGS?=	-Wall -Wno-main

CFLAGS+=	-mcpu=cortex-m4 -msoft-float -mthumb -ffunction-sections -fdata-sections -fno-builtin -fstrict-volatile-bitfields
ifndef NO_LTO
CFLAGS+=	-flto -fno-use-linker-plugin
endif
CPPFLAGS+=	-I${_libdir}/CMSIS/Include -I. -Itoolchain/udad
CPPFLAGS+=	-include ${_libdir}/include/mchck_internal.h

LDFLAGS+=	-Wl,--gc-sections
LDFLAGS+=	-fwhole-program
CPPFLAGS.ld+=	-P -CC -I${_libdir}/ld -I.
CPPFLAGS.ld+=	-DTARGET_LDSCRIPT='"${TARGETLD}"'
LDSCRIPTS+=	${_libdir}/ld/${TARGETLD}
TARGETLD?=	${TARGET}.ld

ifdef LOADER
CPPFLAGS.ld+=	-DMEMCFG_LDSCRIPT='"loader.ld"'
LDSCRIPTS+=	${_libdir}/ld/loader.ld
BINSIZE=	${LOADER_SIZE}
LOADADDR=	${LOADER_ADDR}
else
CPPFLAGS.ld+=	-DMEMCFG_LDSCRIPT='"app.ld"'
LDSCRIPTS+=	${_libdir}/ld/app.ld
BINSIZE=	${APP_SIZE}
LOADADDR=	${APP_ADDR}
endif

LDTEMPLATE=	${PROG}.ld-template
LDFLAGS+=	-T ${LDTEMPLATE}
LDFLAGS+=       -nostartfiles
LDFLAGS+=	-Wl,-Map=${PROG}.map
LDFLAGS+=	-Wl,-output-linker-script=${PROG}.ld

CLEANFILES+=	${PROG}.hex ${PROG}.elf ${PROG}.bin ${PROG}.map

ZSH_RESULT:=$(shell echo ${SRCS})

all: ${PROG}.bin

# This has to go before the rule, because for some reason the updates to OBJS
# are not incorporated into the target dependencies
include ${_libdir}/lib/Makefile.part
$(foreach _uselib,${SRCS.libs},$(eval $(call _include_libs,$(_uselib))))

# linkdep defines LINKOBJS
include ${_libdir}/mk/linkdep.mk

${PROG}.elf: ${LINKOBJS} ${LDLIBS} ${LDTEMPLATE}
	${CC} -o $@ ${CFLAGS} ${LDFLAGS} ${LINKOBJS} ${LDLIBS}

%.bin: %.elf
	@${OBJCOPY} -O binary $< $@.tmp
#	@ls -l $@.tmp | awk '{ s=$$5; as=${BINSIZE}; printf "%d bytes available\n", (as - s); if (s > as) { exit 1; }}'
	@mv $@.tmp $@
CLEANFILES+=	${PROG}.bin.tmp

${LDTEMPLATE}: ${_libdir}/ld/link.ld.S ${LDSCRIPTS}
	${CPP} -o $@ ${CPPFLAGS.ld} $<
CLEANFILES+=	${LDTEMPLATE} ${PROG}.ld

gdb: check-programmer ${PROG}.elf
	${RUBY} ${_libdir}/programmer/gdbserver.rb ${MCHCKADAPTER} -- ${GDB} -readnow -ex 'target extended-remote :1234' ${PROG}.elf

flash: ${PROG}.bin
	${DFUUTIL} -d ${DFUVID}:${DFUPID} -D ${PROG}.bin

swd-flash: check-programmer ${PROG}.bin
	${RUBY} ${_libdir}/programmer/flash.rb ${MCHCKADAPTER} ${PROG}.bin ${LOADADDR}

check-programmer:
#	cd ${_libdir}/.. && git submodule update --init programmer
endif

# from the make info manual
%.d: %.c
	$(GENERATE.d)

ifeq ($(call is-make-clean),)
-include $(patsubst %.o,%.d,${LINKOBJS})
endif

${_objs} $(patsubst %.o,%.d,${_objs}): ${_gensrc}

clean:
	-rm -f ${CLEANFILES}

realclean:
	-rm -f ${REALCLEANFILES} ${CLEANFILES}
