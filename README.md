# Telemetria IoT ponta a ponta na AWS

Um ESP32 físico enviando telemetria simulada para a nuvem, com comunicação autenticada por certificado, processamento em um serviço Node.js e histórico persistido em banco relacional.

A proposta original da atividade era apenas estruturar a comunicação na nuvem. Fui além e integrei o hardware real ao fluxo completo.

---

## Arquitetura

```
┌──────────┐   MQTT + mTLS    ┌───────────────┐   assina tópicos   ┌──────────────┐    INSERT    ┌──────────────┐
│  ESP32   │ ───────────────▶ │ AWS IoT Core  │ ─────────────────▶ │ EC2          │ ───────────▶ │ Amazon RDS   │
│ (C/C++)  │  certificado     │ broker MQTT   │                    │ serviço      │              │ MySQL        │
│          │  X.509           │ + políticas   │                    │ Node.js      │              │ (histórico)  │
└──────────┘                  └───────────────┘                    └──────────────┘              └──────────────┘
```

| Componente | Papel |
|---|---|
| **ESP32** (C/C++) | Conecta ao Wi-Fi, lê os dados e publica a telemetria em um tópico MQTT, autenticando com certificado de dispositivo |
| **AWS IoT Core** | Broker MQTT gerenciado; valida o certificado X.509 do dispositivo e aplica as políticas de acesso por tópico |
| **EC2 + Node.js** | Serviço que assina os tópicos do IoT Core e faz a ponte para o banco |
| **Amazon RDS (MySQL)** | Armazena o histórico de leituras de forma estruturada e persistente |

## Por que mTLS

Em vez de usuário e senha, cada dispositivo tem seu próprio certificado X.509. O IoT Core só aceita a conexão se o certificado for válido e estiver vinculado a uma política que autoriza aquele tópico. Na prática: um dispositivo comprometido pode ser revogado sozinho, sem afetar os outros, e nenhuma credencial trafega em texto.

<!-- TODO: ajuste a estrutura acima para refletir as pastas reais do repositório -->

## Formato da mensagem

<!-- TODO: cole aqui um exemplo real do payload JSON publicado pelo ESP32 -->

```json
{
  "device_id": "esp32-01",
  "timestamp": "2026-06-10T14:32:05Z",
  "...": "..."
}
```

## Como reproduzir

1. **IoT Core:** criar uma *thing*, gerar o certificado do dispositivo e anexar uma política que permita `iot:Connect` e `iot:Publish` no tópico usado.
2. **Firmware:** copiar `firmware/secrets.example.h` para `firmware/secrets.h` e preencher Wi-Fi, *endpoint* do IoT Core e os certificados. Compilar e gravar no ESP32.
3. **Banco:** criar a instância RDS MySQL e rodar `database/schema.sql`.
4. **Serviço:** na EC2, copiar `service/.env.example` para `service/.env`, preencher as credenciais e rodar `npm install && npm start`.

> **Segurança:** certificados, chaves privadas, `.env` e `secrets.h` estão no `.gitignore` e **nunca** devem ser versionados.

## O que aprendi

- Provisionamento de dispositivos e gestão de certificados no AWS IoT Core
- Escrita de políticas de acesso por tópico (princípio do menor privilégio)
- Diferença entre publicar em um broker e consumir de forma confiável do outro lado
- Configuração de instâncias EC2 e RDS e a comunicação segura entre elas

## Tecnologias

ESP32 · C/C++ · MQTT · TLS mútuo (X.509) · AWS IoT Core · Amazon EC2 · Node.js · Amazon RDS · MySQL

## Autor

**Hugo Otávio dos Santos de Paula** — [LinkedIn](https://linkedin.com/in/hugotavios) · [Portfólio](https://hugotavio.com)
