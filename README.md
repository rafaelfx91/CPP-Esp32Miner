⚡ ESP32 SHA-256 Crypto Miner

<p align="center">
<strong>Minerador de criptomoedas completo com interface web e LEDs indicadores, otimizado para o ESP32.</strong>
    

  <em>Mineração real SHA-256 • Interface web responsiva • Sistema dual WiFi para configuração</em>
</p> <p align="center">
  <img src="https://img.shields.io/badge/ESP32-SHA256%20Miner-green?style=for-the-badge&logo=espressif" alt="ESP32">
  <img src="https://img.shields.io/badge/Platform-Arduino%20%7C%20PlatformIO-blue?style=for-the-badge&logo=arduino" alt="Platform">
  <img src="https://img.shields.io/badge/License-MIT-yellow?style=for-the-badge" alt="License">
  <img src="https://img.shields.io/badge/Version-1.0.0-orange?style=for-the-badge" alt="Version">
  <img src="https://img.shields.io/github/stars/seu-usuario/esp32-miner?style=for-the-badge&color=gold" alt="GitHub Stars">
  <img src="https://img.shields.io/github/forks/seu-usuario/esp32-miner?style=for-the-badge&color=lightgrey" alt="GitHub Forks">
</p> <div align="center">   <p>
    🎯 **Mineração Real** • 🌐 **Interface Web** • 📱 **Responsivo** • 💡 **LEDs Indicadores** • 🔧 **Fácil Configuração**
  </p>   <!-- Adicione um GIF ou imagem de demonstração aqui para um impacto visual imediato -->   <!-- <img src="caminho/para/seu/gif-demo.gif" alt="Demonstração do Minerador em Ação" width="600"/> --> </div>




🌟 Destaques do Projeto

Este projeto transforma um microcontrolador ESP32 em um minerador de criptomoedas SHA-256 totalmente funcional. Utilizando o acelerador de hardware do ESP32, ele oferece uma solução de mineração compacta e de baixo consumo, ideal para fins educacionais e experimentais.

Recurso
Descrição Detalhada
⚡ Mineração Real SHA-256
Implementação otimizada que aproveita o acelerador de hardware do ESP32 para o cálculo do Double SHA-256.
🌐 Interface Web Responsiva
Servidor web integrado para controle completo do minerador (configuração, status, reinício) acessível via navegador em qualquer dispositivo.
📶 Sistema Dual WiFi
Alterna automaticamente entre o modo STA (para mineração) e AP (para configuração inicial ou reconfiguração).
💡 LEDs Indicadores Visuais
4 LEDs dedicados para fornecer status em tempo real do dispositivo (Modo AP, Conexão WiFi, Mineração Ativa, Shares Aceitos).
💾 Configuração Persistente
Salva todas as configurações (WiFi, Pool, Carteira) na memória SPIFFS do ESP32, garantindo persistência após reinícios.
🎯 Suporte Multi-Pool (Stratum)
Compatível com pools que utilizam o protocolo Stratum, como Unmineable, permitindo a mineração de diversas moedas.
💰 Suporte Multi-Moedas
Permite a mineração de moedas como TRX, BTC, DOGE e outras, dependendo da pool Stratum configurada (ex: Unmineable).
📊 Monitoramento em Tempo Real
Estatísticas detalhadas de hashrate, shares e uptime disponíveis via API REST e na interface web.





🛠️ Instalação e Configuração Rápida

📋 Pré-requisitos de Hardware

Item
Detalhe
ESP32
Necessário um modelo com suporte ao acelerador SHA-256 (a maioria dos modelos modernos).
4x LEDs
Para indicadores visuais de status.
Fonte de Alimentação
Estável, com capacidade de corrente de ≥500mA para garantir a estabilidade da mineração.
Conexão WiFi
Rede 2.4GHz para conexão à pool de mineração.


📚 Bibliotecas e Software

Este projeto é desenvolvido para Arduino IDE ou PlatformIO.

As bibliotecas necessárias são:

C++


// Bibliotecas necessárias
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include "sha256_acelerado.h" // Biblioteca customizada para aceleração de hardware


🚀 Guia de Início

1.
📥 Clone o Repositório:

2.
🔌 Conexões dos LEDs: Conecte os LEDs aos seguintes pinos GPIO do seu ESP32:

3.
⚙️ Primeira Execução:

•
Carregue o código no seu ESP32 (via Arduino IDE ou PlatformIO).

•
O ESP32 iniciará no modo AP (Access Point) se não encontrar configurações salvas.

•
Conecte-se à rede WiFi ESP32-Miner-Config (senha padrão: 12345678).

•
Acesse o endereço http://192.168.4.1 no seu navegador.

•
Configure sua rede WiFi, dados da pool de mineração e carteira. O dispositivo irá reiniciar e começar a mineração.






⛏️ Detalhes da Mineração

🔄 Protocolo Stratum

O minerador se comunica com a pool utilizando o protocolo Stratum.

mermaid

graph LR
    A[ESP32 Miner] --> B(Conectar à Pool)
    B --> C{Enviar: Subscribe}
    C --> D{Enviar: Authorize}
    D --> E[Pool Envia: Notify - Jobs]
    E --> F[ESP32 Calcula Hash]
    F -- Envia: Submit - Shares --> G[Pool Aceita/Rejeita]
    G --> H[💰 Rewards (se aceito)]

⚡ Algoritmo SHA-256 Otimizado

O coração do projeto é a otimização do algoritmo SHA-256:

•
Aceleração por Hardware: Utiliza a unidade de aceleração criptográfica do ESP32 para o cálculo do hash.

•
Double SHA-256: Realiza o cálculo duplo necessário para a mineração de Bitcoin e moedas compatíveis.

•
Submissão Inteligente: Envia shares para a pool somente após a verificação automática do target.




📊 Performance e Monitoramento

🎯 Métricas Típicas

É importante notar que o ESP32 é um dispositivo de baixo poder.

Métrica
Valor Típico
Descrição
Hashrate
40-60 H/s
Poder de processamento (Hashes por segundo).
Shares/Hora
0.1 - 0.5
Média de shares aceitos pela pool por hora (varia com a dificuldade).
Consumo
~500mA
Consumo elétrico durante a mineração ativa.
Temperatura
45-65°C
Temperatura de operação (monitorar a estabilidade).


🛠️ API Endpoints

A interface web e o monitoramento utilizam os seguintes endpoints REST:

Endpoint
Método
Descrição
/api/stats
GET
📊 Retorna estatísticas em tempo real (hashrate, shares, uptime).
/api/config
GET
⚙️ Retorna a configuração atual salva.
/api/save-config
POST
💾 Salva uma nova configuração (WiFi, Pool, Carteira).
/api/scan-wifi
GET
🔍 Lista as redes WiFi disponíveis.
/api/restart-miner
POST
🔄 Reinicia o minerador.
/api/enable-ap
POST
📶 Força a ativação do modo AP para reconfiguração.
/api/delete-logs
POST
🗑️ Apaga os logs de mineração.





⚠️ Limitações e Considerações

Este projeto é primariamente educacional e experimental.

Aspecto
Detalhe
Rentabilidade
O hashrate de ~50 H/s é muito baixo para ser lucrativo. O foco é no aprendizado e na demonstração de tecnologia.
Consumo de Energia
Embora baixo (~500mA), o consumo contínuo deve ser considerado.
Estabilidade
Requer uma fonte de alimentação estável e uma conexão WiFi robusta para evitar rejeições de shares.
Aquecimento
O ESP32 pode aquecer durante a operação contínua devido ao uso intensivo do acelerador de hardware.





🚨 Solução de Problemas Comuns

Problema
Solução Recomendada
❌ WiFi Não Conecta
1. Verifique se o SSID e a senha estão corretos. 2. Certifique-se de que o sinal WiFi é adequado. 3. Use o Modo AP (/api/enable-ap) para reconfigurar.
❌ Pool Não Conecta
1. Verifique o Host e a Porta da pool. 2. Confirme se a carteira e o nome do worker são válidos. 3. Teste a conectividade de rede do seu roteador.
❌ Baixo Hashrate
1. Lembre-se que o poder do ESP32 é limitado (40-60 H/s). 2. O hashrate varia com a dificuldade da pool. 3. Verifique a estabilidade da fonte de alimentação.





🤝 Contribuições

Contribuições são bem-vindas! Sinta-se à vontade para abrir uma issue ou enviar um Pull Request para:

•
Melhorar a otimização do código.

•
Adicionar novas funcionalidades à interface web.

•
Corrigir bugs.

Licença

Este projeto está licenciado sob a Licença MIT - veja o arquivo LICENSE para mais detalhes.




<p align="center">
Feito com 💖 por [Seu Nome/Usuário]
</p>

