# DreamKai360
Emulador experimental de Dreamcast para Xbox 360 RGH.

## v1.11b — desvios SH-4 portáveis e testes de slot de atraso

O projeto contém um núcleo C experimental em `src/dreamkai360.h` e `src/dreamkai360.c`, com:
- CPU SH-4 interpretada e parcial: operações lógicas, comparações, loads/stores, MOV.B/MOV.W/MOV.L relativos ao PC, MOVA, SWAP, EXTU/EXTS, XTRCT, MOVT, TST, DT, SHAL/SHAR, rotações, deslocamentos, aritmética imediata, CMP/PZ, CMP/PL, transferências STS/LDS e STS.L/LDS.L para MACH/MACL/PR, CLRMAC e desvios BRA/BT/BF com extensão de sinal explícita e slot de atraso experimental; R0 preservado como registrador geral
- MMU/TLB experimental de 64 entradas, ASID e permissões;
- barramento big-endian de 8/16/32 bits e janelas MMIO experimentais;
- DMA por orçamento, IRQ e scheduler;
- framebuffer experimental PVR2 com limpeza, retângulos preenchidos e contagem de quadros;
- mixer de tom de teste, leitura de setores de 2048 bytes de imagem GD-ROM montada em memória e estado básico de entrada Maple.

Os testes de regressão em `tests/test_core.c` cobrem instruções SH-4 selecionadas, loads relativos ao PC, MOVA, desvios positivos/negativos e slot de atraso, comparações assinadas CMP/PZ e CMP/PL, transferências MACH/MACL/PR, incluindo acessos empilhados STS.L/LDS.L, CLRMAC, ADDC/SUBC com carry/borrow e ADDV/SUBV com overflow, MMIO AICA/PVR, permissões da MMU, framebuffer, DMA/IRQ e limites do scheduler.

## Toolchain Xenon na nuvem

O workflow **DreamKai360 Cloud Xenon Toolchain** em `.github/workflows/xenon-cloud.yml` executa os testes host, constrói a toolchain Free60/libXenon e tenta compilar um ELF de teste que vincula o núcleo. A execução #108 concluiu com sucesso em 17/10/2026 para a revisão anterior. A revisão atual adiciona loads relativos ao PC e preserva R0 como registrador geral; aguarde o workflow disparado por estes commits para confirmar a suíte host e o ELF. Consulte [GitHub Actions](https://github.com/MeiKinosaki/DreamKai360/actions) para o resultado atual. Isso não equivale a um `default.xex` nem a teste em console; consulte [GitHub Actions](https://github.com/MeiKinosaki/DreamKai360/actions) para a execução mais recente.

O ELF de teste é uma sonda de compilação e não comprova inicialização no Xbox 360.

## Interrupções — contagem de novas pendências

`dk_irq_raise` mantém os bits pendentes e só incrementa o contador de eventos quando a chamada introduz pelo menos um bit que ainda não estava pendente. Repetir a mesma interrupção sem reconhecê-la não cria uma nova borda; após `dk_irq_ack`, a interrupção pode ser levantada novamente e contabilizada. Isso modela a pendência básica, sem alegar um controlador completo de interrupções SH-4.

## Busca de instruções e permissões de execução

Falhas de busca por desalinhamento ou por ultrapassar o limite da RAM também incrementam o contador de falhas do barramento; as falhas de tradução/permissão continuam sendo contabilizadas pelo caminho da MMU.


A busca de opcode SH-4 usa `dk_bus_fetch16`, caminho separado das leituras de dados. Ele exige alinhamento de 16 bits, aplica a permissão de execução da MMU e lê a instrução da RAM física mapeada. Os testes cobrem uma busca válida e uma tentativa em página não executável, que deve falhar e parar o núcleo. Isso é uma validação do modelo simplificado do projeto, não uma implementação integral de exceções do SH-4.

## Barramento e MMU — cobertura de regressão

Os testes também verificam que a tradução de endereços virtuais da MMU é aplicada pelo barramento, que leituras/escritas chegam à página física mapeada e que uma página somente leitura rejeita escritas e incrementa o contador de falhas. Esta cobertura valida o caminho do barramento em testes de host; ainda não equivale à implementação completa da MMU do Dreamcast nem à execução no Xbox 360 RGH.

## Núcleo SH-4: instruções em progresso

O interpretador também inclui loads indexados `MOV.B/W/L @(R0,Rm),Rn`, com leitura de byte/word/longword pelo barramento e extensão de sinal para byte e word. Testes de regressão cobrem valores negativos, ordenação big-endian e preservação dos registradores de endereço. Isso amplia apenas o subconjunto de instruções; não representa cobertura completa do SH-4.

## Limitações conhecidas

Este núcleo **não é ainda um emulador funcional de Dreamcast**. O SH-4 implementa apenas um subconjunto pequeno de instruções e ainda não possui tratamento completo de exceções, divisão iterativa DIV1 e apenas suporte parcial a registradores especiais MACH/MACL/PR. O mapa físico, alinhamento e MMIO precisam de validação contra o hardware. O PVR2 não rasteriza polígonos e texturas reais; o mixer AICA é apenas um tom de teste; GD-ROM não interpreta imagens CDI/GDI nem implementa todos os comandos do dispositivo; Maple não implementa o protocolo completo. Também faltam BIOS/boot, carregamento de jogos, sincronização completa dos periféricos, integração real de vídeo/áudio libXenon e geração validada de `default.xex`.

As estimativas de progresso não equivalem a compatibilidade comprovada. É necessário passar os testes host, validar a compilação Xenon e depois testar em hardware RGH.

## Executar os testes host

```sh
make clean test
```

O executável de teste host é apenas uma ferramenta de desenvolvimento, não um build para Xbox 360.
