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
- estruturas iniciais para PVR2, AICA, GD-ROM e controle Maple;
- teste host em `tests/test_core.c`, executado pelo alvo `make test` e pelo workflow.

### Limitações conhecidas

Este núcleo **não é ainda um emulador funcional de Dreamcast**. O SH-4 implementa apenas poucas instruções e não tem todos os registradores especiais/exceções; MMU, mapa físico, alinhamento e MMIO precisam de revisão de precisão; PVR2 não rasteriza gráficos, AICA não sintetiza áudio, GD-ROM não lê imagens de disco e Maple ainda não implementa o protocolo. Também faltam BIOS/boot, carregamento de jogos, sincronização completa de periféricos, integração de vídeo/áudio da libXenon e geração de `default.xex`.

As porcentagens são metas de implementação, não garantias de funcionamento. O workflow deve passar primeiro nos testes host; só depois se valida a compilação de um executável para Xenon e, por fim, testes em hardware real.

## Compilar testes no host

```sh
make clean test
```

O executável criado é apenas um teste nativo de desenvolvimento, não um build para Xbox 360.
