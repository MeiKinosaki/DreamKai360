# DreamKai360
Emulador experimental de Dreamcast para Xbox 360 RGH.

## Toolchain Xenon na nuvem

O workflow **DreamKai360 Cloud Xenon Toolchain** em `.github/workflows/xenon-cloud.yml` compila a toolchain Free60/libXenon e gera um ELF de teste. A execução validada anteriormente é [GitHub Actions #22](https://github.com/MeiKinosaki/DreamKai360/actions/runs/38333640835). O ELF dessa execução é apenas uma sonda de toolchain, não o emulador.

## v1.9b — integração inicial do núcleo

Agora o repositório inclui `src/dreamkai360.h` e `src/dreamkai360.c`, um protótipo C integrado com:
- CPU SH-4 interpretada, com subconjunto pequeno de instruções, PC/ciclos e delay slots básico;
- MMU/TLB experimental de 64 entradas, ASID e permissões;
- barramento de RAM e acessos big-endian de 8/16/32 bits, mais janelas básicas de registradores;
- DMA por orçamento, IRQ e scheduler;
- PVR2 experimental com framebuffer externo, limpeza de tela, retângulos preenchidos e contagem de quadros;\n- mixer de áudio de teste em onda quadrada, leitura de setores de 2048 bytes de imagem GD-ROM montada em memória e estado de entrada Maple;
- teste host em `tests/test_core.c`, executado pelo alvo `make test` e pelo workflow.

### Limitações conhecidas

Este núcleo **não é ainda um emulador funcional de Dreamcast**. O SH-4 implementa apenas poucas instruções e não tem todos os registradores especiais/exceções; MMU, mapa físico, alinhamento e MMIO precisam de revisão de precisão; PVR2 ainda não rasteriza polígonos/texturas Dreamcast nem usa a pipeline PowerVR2 real; o mixer AICA é apenas um tom de teste, sem síntese de vozes/SPU; GD-ROM lê setores de uma imagem já montada em memória, sem parser CDI/GDI e sem comandos ATAPI completos; Maple aceita estado do controle por API, sem protocolo de barramento completo. Também faltam BIOS/boot, carregamento de jogos, sincronização completa de periféricos, integração de vídeo/áudio da libXenon e geração de `default.xex`.

As porcentagens são metas de implementação, não garantias de funcionamento. O workflow deve passar primeiro nos testes host; só depois se valida a compilação de um executável para Xenon e, por fim, testes em hardware real.

## Compilar testes no host

```sh
make clean test
```

O executável criado é apenas um teste nativo de desenvolvimento, não um build para Xbox 360.
