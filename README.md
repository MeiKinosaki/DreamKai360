# DreamKai360
Emulador experimental de Dreamcast para Xbox 360 RGH.

## v1.11b — desvios SH-4 portáveis e testes de slot de atraso

O projeto contém um núcleo C experimental em `src/dreamkai360.h` e `src/dreamkai360.c`, com:
- CPU SH-4 interpretada e parcial: operações lógicas, comparações, loads/stores, SWAP, EXTU/EXTS, XTRCT, MOVT, TST, DT, SHAL/SHAR, rotações, deslocamentos, aritmética imediata, CMP/PZ, CMP/PL e desvios BRA/BT/BF com extensão de sinal explícita e slot de atraso experimental;
- MMU/TLB experimental de 64 entradas, ASID e permissões;
- barramento big-endian de 8/16/32 bits e janelas MMIO experimentais;
- DMA por orçamento, IRQ e scheduler;
- framebuffer experimental PVR2 com limpeza, retângulos preenchidos e contagem de quadros;
- mixer de tom de teste, leitura de setores de 2048 bytes de imagem GD-ROM montada em memória e estado básico de entrada Maple.

Os testes de regressão em `tests/test_core.c` cobrem instruções SH-4 selecionadas, desvios positivos/negativos e slot de atraso, comparações assinadas CMP/PZ e CMP/PL, MMIO AICA/PVR, permissões da MMU, framebuffer, DMA/IRQ e limites do scheduler.

## Toolchain Xenon na nuvem

O workflow **DreamKai360 Cloud Xenon Toolchain** em `.github/workflows/xenon-cloud.yml` executa os testes host, constrói a toolchain Free60/libXenon e tenta compilar um ELF de teste que vincula o núcleo. A execução #83 passou incluindo testes host, construção da toolchain Free60/libXenon, runtime e compilação/auditoria do ELF de teste. Isso não equivale a um `default.xex` nem a teste em console; consulte [GitHub Actions](https://github.com/MeiKinosaki/DreamKai360/actions) para a execução mais recente.

O ELF de teste é uma sonda de compilação e não comprova inicialização no Xbox 360.

## Limitações conhecidas

Este núcleo **não é ainda um emulador funcional de Dreamcast**. O SH-4 implementa apenas um subconjunto pequeno de instruções e ainda não possui tratamento completo de exceções e registradores especiais. O mapa físico, alinhamento e MMIO precisam de validação contra o hardware. O PVR2 não rasteriza polígonos e texturas reais; o mixer AICA é apenas um tom de teste; GD-ROM não interpreta imagens CDI/GDI nem implementa todos os comandos do dispositivo; Maple não implementa o protocolo completo. Também faltam BIOS/boot, carregamento de jogos, sincronização completa dos periféricos, integração real de vídeo/áudio libXenon e geração validada de `default.xex`.

As estimativas de progresso não equivalem a compatibilidade comprovada. É necessário passar os testes host, validar a compilação Xenon e depois testar em hardware RGH.

## Executar os testes host

```sh
make clean test
```

O executável de teste host é apenas uma ferramenta de desenvolvimento, não um build para Xbox 360.
