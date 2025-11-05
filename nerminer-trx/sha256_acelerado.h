#ifndef SHA256_ACELERADO_H
#define SHA256_ACELERADO_H

#include <Arduino.h>
#include "soc/hwcrypto_reg.h"
#include "soc/dport_reg.h"
#include "esp32/rom/ets_sys.h"

// Macro para inverter a ordem dos bytes (necessário para Bitcoin/Stratum)
#define BYTESWAP32(z) ((uint32_t)((z&0xFF)<<24|((z>>8)&0xFF)<<16|((z>>16)&0xFF)<<8|((z>>24)&0xFF)))

// Endereço base dos registradores SHA
#define SHA_TEXT_BASE 0x3FF0F000

// Função para calcular o Double SHA-256 de um cabeçalho de bloco de 80 bytes
// O resultado (32 bytes) é armazenado em 'output_hash'
void calculate_double_sha256(uint8_t *header_data, uint8_t *output_hash) {
    // Os registradores SHA256 são usados para entrada e saída no ESP32
    volatile uint32_t *shaData = (uint32_t*) SHA_TEXT_BASE;
    volatile uint32_t *hash = (uint32_t*) SHA_TEXT_BASE;

    // 1. HASH SHA-256 (Primeira rodada)
    
    // Copia os 64 bytes iniciais do cabeçalho para os registradores de dados SHA
    // O cabeçalho de 80 bytes é composto por 20 palavras de 4 bytes (uint32_t)
    uint32_t *header32 = (uint32_t*) header_data;
    
    // Copia as primeiras 16 palavras (64 bytes)
    for(int i = 0; i < 16; i++) {
        shaData[i] = header32[i];
    }

    // Inicia um novo hash
    DPORT_REG_WRITE(SHA_256_START_REG, 1);
    while(DPORT_REG_READ(SHA_256_BUSY_REG) != 0);

    // Copia as 4 palavras restantes (16 bytes) do cabeçalho
    shaData[0] = header32[16];
    shaData[1] = header32[17];
    shaData[2] = header32[18];
    shaData[3] = header32[19];
    
    // Preenchimento (Padding)
    shaData[4] = 0x80000000; // Trailing bit
    shaData[5] = 0;
    shaData[6] = 0;
    shaData[7] = 0;
    shaData[8] = 0;
    shaData[9] = 0;
    shaData[10] = 0;
    shaData[11] = 0;
    shaData[12] = 0;
    shaData[13] = 0;
    shaData[14] = 0;
    shaData[15] = 0x00000280; // Tamanho total em bits (80 bytes * 8 = 640 bits)

    // Continua o hash
    DPORT_REG_WRITE(SHA_256_CONTINUE_REG, 1);
    while(DPORT_REG_READ(SHA_256_BUSY_REG) != 0);

    // Carrega o resultado do primeiro hash nos registradores
    DPORT_REG_WRITE(SHA_256_LOAD_REG, 1);
    while(DPORT_REG_READ(SHA_256_BUSY_REG) != 0);

    // O resultado do primeiro hash (32 bytes) está agora nos primeiros 8 registradores (hash[0] a hash[7])
    
    // 2. HASH SHA-256 (Segunda rodada)
    
    // O resultado do primeiro hash já está nos registradores de dados (shaData[0] a shaData[7])
    
    // Preenchimento (Padding) para a segunda rodada
    shaData[8] = 0x80000000; // Trailing bit
    shaData[9] = 0;
    shaData[10] = 0;
    shaData[11] = 0;
    shaData[12] = 0;
    shaData[13] = 0;
    shaData[14] = 0;
    shaData[15] = 0x00000100; // Tamanho total em bits (32 bytes * 8 = 256 bits)

    // Inicia um novo hash
    DPORT_REG_WRITE(SHA_256_START_REG, 1);
    while(DPORT_REG_READ(SHA_256_BUSY_REG) != 0);

    // Carrega o resultado final nos registradores
    DPORT_REG_WRITE(SHA_256_LOAD_REG, 1);
    while(DPORT_REG_READ(SHA_256_BUSY_REG) != 0);

    // Copia o resultado final (32 bytes) para o buffer de saída
    memcpy(output_hash, (uint8_t*)hash, 32);
}

// Função para inicializar o acelerador SHA-256
void init_sha256_accelerator() {
    // Habilita o clock do periférico SHA
    DPORT_REG_SET_BIT(DPORT_PERI_CLK_EN_REG, DPORT_PERI_EN_SHA);
    // Remove o reset do periférico SHA
    DPORT_REG_CLR_BIT(DPORT_PERI_RST_EN_REG, DPORT_PERI_EN_SHA);
}

#endif // SHA256_ACELERADO_H
