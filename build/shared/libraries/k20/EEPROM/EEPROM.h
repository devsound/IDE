#ifndef __UDAD_EEPROM__
#define __UDAD_EEPROM__
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void eeprom_initialize(void);
uint8_t eeprom_read_byte(const uint8_t *addr);
uint16_t eeprom_read_word(const uint16_t *addr);
uint32_t eeprom_read_dword(const uint32_t *addr);
void eeprom_read_block(void *buf, const void *addr, uint32_t len);
int eeprom_is_ready(void);
static void flexram_wait(void);
void eeprom_write_byte(uint8_t *addr, uint8_t value);
void eeprom_write_word(uint16_t *addr, uint16_t value);
void eeprom_write_dword(uint32_t *addr, uint32_t value);
void eeprom_write_block(const void *buf, void *addr, uint32_t len);

#ifdef __cplusplus
}

static class {
  public:

    // Extensions
    static inline void write8(unsigned int address, uint8_t value) { eeprom_write_byte((uint8_t *) address, value); }
    static inline void write16(unsigned int address, uint16_t value) { eeprom_write_word((uint16_t *) address, value); }
    static inline void write32(unsigned int address, uint32_t value) { eeprom_write_dword((uint32_t *) address, value); }
    static inline void writeBytes(unsigned int address, const void *data, size_t len) { eeprom_write_block(data, (void *) address, (uint32_t)len); }
    static inline uint8_t read8(unsigned int address) { return eeprom_read_byte((uint8_t *) address); }
    static inline uint16_t read16(unsigned int address) { return eeprom_read_word((uint16_t *) address); }
    static inline uint32_t read32(unsigned int address) { return eeprom_read_dword((uint32_t *) address); }
    static inline void readBytes(unsigned int address, void *data, size_t len) { eeprom_read_block(data, (const void *) address, (uint32_t)len); }

    // Traditional Arduino aliases
    static inline void write(unsigned int address, uint8_t value) { write8(address, value); }
    static inline uint8_t read(unsigned int address) { return read8(address); }
    
    // Extended Arduino-style aliases
    static inline void writeChar(unsigned int address, uint8_t value) { write8(address, value); }
    static inline void writeInt(unsigned int address, uint16_t value) { write16(address, value); }
    static inline void writeLong(unsigned int address, uint32_t value) { write32(address, value); }
    static inline uint8_t readChar(unsigned int address) { return read8(address); }
    static inline uint16_t readInt(unsigned int address) { return read16(address); }
    static inline uint32_t readLong(unsigned int address) { return read32(address); }
} EEPROM;

#endif

#endif//__UDAD_EEPROM__