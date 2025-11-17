# Estação de Irrigação IoT (ESP32 + SinricPro)

Descrição e instruções do sketch `ESP32_Sinric_Plant_Watering_Final.ino`.

---

## Visão Geral

Este projeto implementa uma estação de irrigação e monitoramento ambiental usando um ESP32. Ele monitora:
- Umidade do solo (entrada analógica)
- Temperatura e umidade do ar (sensor DHT22)

E controla:
- Uma bomba d'água via módulo de relé

O dispositivo oferece uma interface web com visualização em tempo real e integração com Alexa/SinricPro para controle por voz.

## Principais arquivos
- `ESP32_Sinric_Plant_Watering_Final.ino` — sketch principal (web UI, leitura de sensores, SinricPro)
- `README.md` — este arquivo

## Recursos
- Interface web responsiva com gauges de solo, temperatura e umidade.
- Botão na interface para alternar a bomba.
- Endpoint `/data` que retorna JSON com leituras e estado atual.
- Integração com SinricPro para controle por voz (dispositivo tipo Switch).

## Hardware necessário
- ESP32 (qualquer placa compatível com ADC 12-bit)
- Módulo relé (ver observações sobre Active LOW / Active HIGH)
- Sensor de umidade do solo (sonda resistiva ou similar)
- Sensor DHT22 (temperatura e umidade)
- Fonte e fiação apropriadas para a bomba (não conectar a bomba diretamente ao ESP!)

## Conexões (exemplo usado no sketch)
- `RELAY_PIN` -> GPIO 12 (para controle do relé)
- `SOIL_PIN` -> GPIO 34 (entrada analógica do sensor de solo)
- `DHT_PIN` -> GPIO 27 (DHT22)

Observação: ajuste os pinos conforme sua placa.

## Configuração do software
1. Abra `ESP32_Sinric_Plant_Watering_Final.ino`.
2. Preencha as credenciais Wi‑Fi:

```cpp
#define SSID "SEU_SSID"
#define PASS "SUA_SENHA"
```

3. Preencha as credenciais do SinricPro (no sketch):

```cpp
#define APP_KEY    "SEU-APP-KEY"
#define APP_SECRET "SEU-APP-SECRET"
#define SWITCH_ID  "SEU-DEVICE-ID"
```

4. Compile e faça o upload para o ESP32.

## Endpoints HTTP disponíveis
- `/` — Página web principal (HTML/CSS/JS integrada no sketch)
- `/data` — Retorna JSON com o estado e leituras: `{ pumpState, soilMoisture, temperature, humidity }`
- `/toggle_pump` — Alterna o estado da bomba (usado pelo botão na página web)

## Como a interface web interpreta os valores
- `soilMoisture` é o valor bruto do ADC do ESP32 (0–4095). A página converte para porcentagem com a fórmula usada no JS:

```js
const moisturePercent = 100 - ((data.soilMoisture / 4095) * 100);
```

Isso pressupõe que leituras maiores representam solo mais seco (ajuste conforme seu sensor).

## Importante — Relés e lógica de acionamento
O sketch usa `digitalWrite(RELAY_PIN, pumpState)` onde `pumpState` é `true`/`false`. Em termos elétricos:
- `LOW` -> nível lógico 0V
- `HIGH` -> nível lógico 3.3V

Muitos módulos de relé são "Active LOW" (são ativados quando o pino é `LOW`). Dependendo do seu módulo, o comportamento físico pode ser invertido em relação ao `pumpState` lógico. Teste manualmente e, se necessário, adapte o sketch para um mapeamento explícito, por exemplo:

```cpp
const bool RELAY_ACTIVE_LOW = true;
void applyPumpState(bool state) {
  if (RELAY_ACTIVE_LOW) digitalWrite(RELAY_PIN, state ? LOW : HIGH);
  else digitalWrite(RELAY_PIN, state ? HIGH : LOW);
}
```

## Sugestões e melhorias
- Tratar `NaN` nas leituras do DHT e retornar JSON consistente (ex.: `null`).
- Implementar calibração do sensor de solo (valores secos/molhados) e mapear corretamente para porcentagem.
- Prevenir ciclos rápidos liga/desliga na bomba (histerese ou tempo mínimo ligado/desligado).
- Adicionar autenticação básica à interface web se exposta na rede.
- Persistir histórico de leituras localmente ou enviar para serviço de nuvem (ThingSpeak, InfluxDB, etc.).

## Segurança e cuidados
- Nunca alimente a bomba diretamente do ESP32; use o relé corretamente isolado e uma fonte separada.
- Verifique aterramento e corrente da bomba.

## Testes rápidos
1. Faça upload do sketch e abra o Serial Monitor (115200 bps).
2. Verifique se o ESP conecta na sua rede Wi‑Fi.
3. Abra o IP mostrado no Serial no navegador para visualizar o painel.

## Referências no projeto
- Arquivo principal: `ESP32_Sinric_Plant_Watering_Final.ino`

---

Se quiser, eu posso:
- Ajustar o sketch para suportar `RELAY_ACTIVE_LOW` configurável.
- Melhorar o JSON retornado por `/data` para lidar com leituras inválidas.
- Adicionar autenticação simples à interface web.

Coloquei este README na mesma pasta do sketch. Se desejar, adapto o conteúdo para o `README.md` raiz do repositório.
