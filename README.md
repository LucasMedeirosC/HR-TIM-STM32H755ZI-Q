# HRTIM com update de CMP1 e CMP2 via DMA

Este documento resume a configuracao atual do sistema no core CM7 para atualizar os comparadores `CMP1` e `CMP2` do Timer A via burst DMA do HRTIM, usando os parametros aplicados no `h755_hrtim.ioc`.

## 1) Arquivos principais envolvidos

- `CM7/Core/Inc/stm32h7xx_hal_conf.h`
  - Habilitou o modulo HAL do HRTIM:
  - `#define HAL_HRTIM_MODULE_ENABLED`
- `CM7/Core/Inc/hrtim.h`
  - Declarou o buffer de DMA compartilhado com `main.c`:
  - `extern uint32_t hrtim_dma_buffer[2];`
- `CM7/Core/Src/hrtim.c`
  - Configura o HRTIM1, o burst DMA do Timer A e as saidas TA1/TA2.
- `CM7/Core/Src/main.c`
  - Mantem o buffer `hrtim_dma_buffer[2]` com os novos valores de `CMP1` e `CMP2`.
  - Inicializa o HRTIM, limpa o D-Cache do buffer e arma a transferencia DMA.

## 2) Fluxo de atualizacao via DMA

O sistema atual nao escreve diretamente em `CMP1` e `CMP2` a cada ciclo. Em vez disso:

1. `main.c` calcula os valores desejados de comparacao.
2. Esses valores sao gravados em `hrtim_dma_buffer[0]` e `hrtim_dma_buffer[1]`.
3. Antes de iniciar a transferencia, o CM7 limpa o D-Cache do buffer com `SCB_CleanDCache_by_Addr(...)`.
4. O HRTIM usa burst DMA para copiar os dois valores para `BDMADR`, atualizando os registros do Timer A.
5. O DMA trabalha em modo circular, entao a atualizacao pode ser repetida continuamente.

> No CM7, com D-Cache habilitado, manter a limpeza do cache antes da transferencia DMA e essencial para o hardware enxergar os valores novos.

### Diagrama do fluxo

```mermaid
flowchart LR
  app[main.c] --> buf[hrtim_dma_buffer\nCMP1 / CMP2]
  app --> cache[SCB_CleanDCache_by_Addr]
  cache --> dmaH[HRTIM burst DMA\nDMA1_Stream1]
  buf --> dmaH
  dmaH --> bdmadr[HRTIM1 BDMADR]
  bdmadr --> cmp[Timer A\nCMP1 / CMP2]
  cmp --> out[TA1 / TA2\nPWM + dead time]

  cmp3[CMP3 do Timer A] --> trig[HRTIM_ADCTRIGGEREVENT13_TIMERA_CMP3]
  trig --> adc[ADC1]
  adc --> dmaA[DMA1_Stream0]
  dmaA --> adcbuf[adc_buffer[0]]
```

## 3) Configuracao aplicada no `.ioc`

### HRTIM / Timer A

- `HRTIM.Periode_TA = 0x1388`
- `HRTIM.PrescalerRatio_TA = HRTIM_PRESCALERRATIO_DIV1`
- `HRTIM.CompareUnit1-Output_TA1TA2 = HRTIM_COMPAREUNIT_1`
- `HRTIM.CompareUnit2-Output_TA1TA2 = HRTIM_COMPAREUNIT_2`
- `HRTIM.CompareUnit3-Output_TA1TA2 = HRTIM_COMPAREUNIT_3`
- `HRTIM.CompareValue1-Output_TA1TA2 = 0x1F4`
- `HRTIM.CompareValue2-Output_TA1TA2 = 0x1194`
- `HRTIM.CompareValue3-Output_TA1TA2 = 0x960`
- `HRTIM.DeadTimeInsertion-Output_TA1TA2 = HRTIM_TIMDEADTIMEINSERTION_ENABLED`
- `HRTIM.RisingValue-Output_TA1TA2 = 0x28`
- `HRTIM.FallingValue-Output_TA1TA2 = 0x28`
- `HRTIM.PreloadEnable-Output_TA1TA2 = HRTIM_PRELOAD_ENABLED`
- `HRTIM.UpdateGating-Output_TA1TA2 = HRTIM_UPDATEGATING_DMABURST`
- `HRTIM.RepetitionUpdate-Output_TA1TA2 = HRTIM_UPDATEONREPETITION_ENABLED`
- `HRTIM.BurstDMAconfig-Output_TA1TA2 = HRTIM_TIMERINDEX_TIMER_A`
- `HRTIM.NumberRegisters_BURSTDMA-Output_TA1TA2 = 2`
- `HRTIM.RegistersToUpdate_Source1-Output_TA1TA2 = HRTIM_BURSTDMA_CMP1`
- `HRTIM.RegistersToUpdate_Source2-Output_TA1TA2 = HRTIM_BURSTDMA_CMP2`
- `HRTIM.DMARequests1-Output_TA1TA2 = HRTIM_TIM_DMA_UPD`
- `HRTIM.DMASrcAddress-Output_TA1TA2 = (uint32_t)hrtim_dma_buffer`
- `HRTIM.DMADstAddress-Output_TA1TA2 = (uint32_t)&hhrtim.Instance->sCommonRegs.BDMADR`
- `HRTIM.DMASize-Output_TA1TA2 = 0x2`

### DMA do HRTIM

- `Dma.HRTIM1_A.1.Instance = DMA1_Stream1`
- `Dma.HRTIM1_A.1.Direction = DMA_MEMORY_TO_PERIPH`
- `Dma.HRTIM1_A.1.PeriphDataAlignment = DMA_PDATAALIGN_WORD`
- `Dma.HRTIM1_A.1.MemDataAlignment = DMA_MDATAALIGN_WORD`
- `Dma.HRTIM1_A.1.MemInc = DMA_MINC_ENABLE`
- `Dma.HRTIM1_A.1.PeriphInc = DMA_PINC_DISABLE`
- `Dma.HRTIM1_A.1.Mode = DMA_CIRCULAR`
- `Dma.HRTIM1_A.1.Priority = DMA_PRIORITY_LOW`
- `Dma.HRTIM1_A.1.RequestNumber = 1`
- `Dma.HRTIM1_A.1.EventEnable = DISABLE`

### ADC sincronizado com o HRTIM

- `ADC1.Channel-0#ChannelRegularConversion = ADC_CHANNEL_19`
- `ADC1.Rank-0#ChannelRegularConversion = 1`
- `ADC1.SamplingTime-0#ChannelRegularConversion = ADC_SAMPLETIME_16CYCLES_5`
- `ADC1.ExternalTrigConv = ADC_EXTERNALTRIG_HR1_ADCTRG1`
- `ADC1.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING`
- `ADC1.ConversionDataManagement = ADC_CONVERSIONDATA_DMA_CIRCULAR`
- `ADC1.NbrOfConversion = 1`
- `Dma.ADC1.0.Instance = DMA1_Stream0`
- `Dma.ADC1.0.Direction = DMA_PERIPH_TO_MEMORY`
- `Dma.ADC1.0.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD`
- `Dma.ADC1.0.MemDataAlignment = DMA_MDATAALIGN_HALFWORD`
- `Dma.ADC1.0.MemInc = DMA_MINC_ENABLE`
- `Dma.ADC1.0.PeriphInc = DMA_PINC_DISABLE`
- `Dma.ADC1.0.Mode = DMA_CIRCULAR`
- `Dma.ADC1.0.Priority = DMA_PRIORITY_HIGH`

O disparo do ADC e sincronizado com o evento `HRTIM_ADCTRIGGEREVENT13_TIMERA_CMP3`, configurado em `HRTIM.ADCTrigger1_Source1`. Na pratica, cada vez que o Timer A atinge `CMP3`, o HRTIM gera o trigger para iniciar uma conversao do ADC1.

No codigo:

1. `MX_ADC1_Init()` configura o canal 19, o trigger externo e o DMA circular.
2. `Iniciar_ADC_DMA()` faz a calibracao e chama `HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_buffer, 1)`.
3. As amostras sao armazenadas em `adc_buffer[0]`.

Assim, o caminho do ADC fica acoplado ao ciclo do PWM e a leitura acontece sempre sincronizada com o instante definido pelo `CMP3`.

## 4) Calculo da frequencia e periodo

Objetivo: 40 kHz com clock de 200 MHz.

Formula:

- `Periodo = f_clk / f_pwm`
- `Periodo = 200.000.000 / 40.000 = 5.000`

Valor configurado em `hrtim.c`:

- `pTimeBaseCfg.Period = 0x1388` (decimal 5000)

## 5) Calculo dos comparadores (10% e 90%)

Com periodo total de 5000:

- Compare 1 (10%):
  - `5000 x 0,10 = 500`
  - Hexadecimal: `0x01F4`
- Compare 2 (90%):
  - `5000 x 0,90 = 4500`
  - Hexadecimal: `0x1194`

Valores aplicados em `hrtim.c`:

- `pCompareCfg.CompareValue = 0x1F4` para `HRTIM_COMPAREUNIT_1`
- `pCompareCfg.CompareValue = 0x1194` para `HRTIM_COMPAREUNIT_2`

## 6) Calculo do dead time (200 ns)

Clock do HRTIM: 200 MHz

- `T_tick = 1 / 200.000.000 = 5 ns`
- `Valor_deadtime = 200 ns / 5 ns = 40`

Hexadecimal de 40:

- `0x28`

Valores configurados em `hrtim.c`:

- `pDeadTimeCfg.RisingValue = 0x28`
- `pDeadTimeCfg.FallingValue = 0x28`
- Prescaler do dead time em DIV1.

## 7) Forma de onda configurada

No Timer A:

- Saida TA1:
  - `SetSource = HRTIM_OUTPUTSET_TIMCMP1`
  - `ResetSource = HRTIM_OUTPUTRESET_TIMCMP2`
- Saida TA2:
  - `SetSource = HRTIM_OUTPUTSET_NONE`
  - `ResetSource = HRTIM_OUTPUTRESET_NONE`
  - Escolha intencional: com `DeadTimeInsertion` habilitado no Timer A, o TA2 e utilizado como complementar do TA1 com insercao de dead time.

Interpretacao da TA1:

- Sobe no compare 1 (10% do periodo) e desce no compare 2 (90% do periodo).
- Isso gera pulso ativo de 80% do periodo, deslocado dentro do ciclo (de 10% ate 90%).

Interpretacao da TA2:

- Embora esteja com Set/Reset em NONE, ela foi iniciada para operar como saida complementar da TA1 quando a unidade de dead time do Timer A esta ativa.

## 8) GPIO e clock do periferico

Configuracoes realizadas no MSP (`HAL_HRTIM_MspInit` e `HAL_HRTIM_MspPostInit`):

- Clock do HRTIM1 habilitado.
- Clock de GPIOC habilitado.
- Pinos configurados:
  - PC6 -> HRTIM_CHA1
  - PC7 -> HRTIM_CHA2
  - Alternate Function: `GPIO_AF1_HRTIM1`

## 9) Sequencia de inicializacao na `main`

No `CM7/Core/Src/main.c`, a sequencia relevante ficou:

1. `HAL_Init()`
2. `SystemClock_Config()`
3. `MX_GPIO_Init()`
4. `MX_DMA_Init()`
5. `MX_HRTIM_Init()`
6. `MX_ADC1_Init()`
7. `Iniciar_ADC_DMA()`
8. `__HAL_HRTIM_TIMER_CLEAR_IT(...)` e `__HAL_HRTIM_TIMER_ENABLE_IT(...)`
9. `Iniciar_Modulacao_HRTIM()`

Dentro de `Iniciar_Modulacao_HRTIM()`:

1. limpa o D-Cache do buffer `hrtim_dma_buffer`
2. inicia o DMA apontando de `hrtim_dma_buffer` para `BDMADR`
3. habilita a requisicao DMA do Timer A
4. inicia as saidas `TA1` e `TA2`
5. inicia o contador do Timer A

Ou seja: primeiro o buffer e atualizado em memoria, depois o HRTIM le esses dois valores por DMA e atualiza `CMP1` e `CMP2` de forma ciclica.

## 10) Como atualizar `CMP1` e `CMP2` em runtime

Para mudar o duty em tempo de execucao, basta recalcular os dois valores e gravar no buffer:

```c
hrtim_dma_buffer[0] = novo_cmp1;
hrtim_dma_buffer[1] = novo_cmp2;
SCB_CleanDCache_by_Addr((uint32_t *)hrtim_dma_buffer, sizeof(hrtim_dma_buffer));
```

Se o fluxo de DMA estiver em execucao, os novos valores vao ser consumidos na proxima transferencia do burst DMA.

## 11) Referencias diretas no codigo

- `CM7/Core/Inc/stm32h7xx_hal_conf.h`: habilitacao do modulo HRTIM.
- `CM7/Core/Src/hrtim.c`: configuracao do burst DMA, periodo, comparadores, dead time e saidas.
- `CM7/Core/Src/main.c`: buffer de DMA, limpeza de cache e sequencia de start.

## 12) Como alterar a forma do PWM (passo a passo)

Se quiser alterar a forma do PWM em runtime, atualize os dois valores do buffer e deixe o DMA propagá-los ao HRTIM:

1. Crie e zere a estrutura `HRTIM_CompareCfgTypeDef`.
2. Calcule `CMP1` e `CMP2`.
3. Grave os valores em `hrtim_dma_buffer[0]` e `hrtim_dma_buffer[1]`.
4. Chame `SCB_CleanDCache_by_Addr(...)` antes da proxima transferencia DMA.

Exemplo:

```c
hrtim_dma_buffer[0] = 500;
hrtim_dma_buffer[1] = 4500;
SCB_CleanDCache_by_Addr((uint32_t *)hrtim_dma_buffer, sizeof(hrtim_dma_buffer));
```

Observacao:

- Para manter o formato atual da onda, ajuste `CMP1` e `CMP2` de forma coordenada.
