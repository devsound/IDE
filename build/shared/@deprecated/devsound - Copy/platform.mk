ifeq (${PLATFORM},MCHCK)
include toolchain/mchck.mk
endif

ifeq (${DEVICE},UDAD_R001)
DEVICE=UDAD
CPPFLAGS+= -DUDAD_REV=1
endif

ifeq (${DEVICE},UDAD_R004)
DEVICE=UDAD
CPPFLAGS+= -DUDAD_REV=4
endif

ifeq (${PLATFORM},UDAD)
include toolchain/udad.mk
endif
