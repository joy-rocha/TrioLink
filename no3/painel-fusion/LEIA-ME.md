# Painel Fusion (back-end + painel)

O servidor (Node.js) conecta no broker por MQTT com TLS (porta 8883, CA `ca.crt`, usuário e senha),
grava tudo em `data/historico.txt` e entrega os dados ao painel em tempo real.

## Como rodar
1. Instale o Node.js 18+ (https://nodejs.org).
3. Abra http://localhost:3000. Outros PCs da rede usam http://IP_DESTE_PC:3000.

## Configuração (`config.json`)
- `broker.host` / `port`: endereço e porta do broker (8883).
- `broker.verifyHostname`: `false` aceita o certificado do broker mesmo que o IP não esteja nele
  (a CA continua sendo validada). Se o certificado do broker tiver o IP no SAN, use `true`.
- `web.port`: porta do painel (3000).

## Histórico
Fica em `data/historico.txt` (colunas separadas por tab), mesmo com o painel fechado.
O botão "Baixar histórico" do painel baixa esse arquivo; "Limpar histórico" apaga.
