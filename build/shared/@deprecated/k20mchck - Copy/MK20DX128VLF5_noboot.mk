# MK20DX128VLF5 without bootloader
TARGET_FAMILY=		K20

FIXED_SECTIONS+=	-s 0:.isr_vector
FIXED_SECTIONS+=	-s 0x400:.flash_config

LOADER=		yes
LOADER_SIZE=	131072
LOADER_ADDR=	0
APP_SIZE=	0
APP_ADDR=	131072
