#ifndef SHA256_ACELERADO_H
#define SHA256_ACELERADO_H

#include <Arduino.h>
#include "soc/hwcrypto_reg.h"
#include "soc/dport_reg.h"

#define SHA_TEXT_BASE 0x3FF0F000

// Função corrigida para funcionar em todas as versões do core ESP32
inline void calculate_double_sha256(uint8_t *header_data, uint8_t *output_hash) {
    volatile uint32_t *regs = (volatile uint32_t*) SHA_TEXT_BASE;
    uint32_t *header32 = (uint32_t*) header_data;

    // === PRIMEIRA PASSADA (80 bytes → 32 bytes) ===
    for (int i = 0; i < 16; i++) regs[i] = header32[i];           // 64 bytes
    DPORT_REG_WRITE(SHA_256_START_REG, 1);
    while (DPORT_REG_READ(SHA_256_BUSY_REG));

    regs[0] = header32[16];
    regs[1] = header32[17];
    regs[2] = header32[18];
    regs[3] = header32[19];

    regs[4]  = 0x80000000;
    for (int i = 5; i < 15; i++) regs[i] = 0;                     // zera 5..14
    regs[15] = 0x00000280;                                        // 640 bits

    DPORT_REG_WRITE(SHA_256_CONTINUE_REG, 1);
    while (DPORT_REG_READ(SHA_256_BUSY_REG));
    DPORT_REG_WRITE(SHA_256_LOAD_REG, 1);
    while (DPORT_REG_READ(SHA_256_BUSY_REG));

    // === SEGUNDA PASSADA (32 bytes → hash final) ===
    regs[8]  = 0x80000000;
    for (int i = 9; i < 15; i++) regs[i] = 0;                     // zera 9..14
    regs[15] = 0x00000100;                                        // 256 bits

    DPORT_REG_WRITE(SHA_256_CONTINUE_REG, 1);  // AQUI ESTAVA O ERRO ANTERIOR!
    while (DPORT_REG_READ(SHA_256_BUSY_REG));
    DPORT_REG_WRITE(SHA_256_LOAD_REG, 1);
    while (DPORT_REG_READ(SHA_256_BUSY_REG));

    // Copia o hash final (os primeiros 8 words = 32 bytes)
    for (int i = 0; i < 8; i++) {
        ((uint32_t*)output_hash)[i] = regs[i];
    }
}

void init_sha256_accelerator() {
    DPORT_REG_SET_BIT(DPORT_PERI_CLK_EN_REG, DPORT_PERI_EN_SHA);
    DPORT_REG_CLR_BIT(DPORT_PERI_RST_EN_REG, DPORT_PERI_EN_SHA);
}

#endif