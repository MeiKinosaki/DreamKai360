# DreamKai360
Emulador de Dreamcast para Xbox 360.

## Toolchain Xenon na nuvem

O workflow **DreamKai360 Cloud Xenon Toolchain** está em `.github/workflows/xenon-cloud.yml`. Ele usa GitHub Actions para instalar as dependências, obter o código upstream do Free60Project/libxenon, compilar o compilador Xenon e a biblioteca libXenon, e produzir um ELF de teste.

### Estado validado

A execução [#22 do GitHub Actions](https://github.com/MeiKinosaki/DreamKai360/actions/runs/38333640835) concluiu com sucesso em 13/10/2026. O log confirma que as etapas de construção do compilador Xenon, construção da libXenon e compilação/auditoria do ELF de teste terminaram sem falha.

O artefato `DreamKai360-cloud-xenon-sdk-22` contém:
- `cloud/dreamkai360_cloud_probe.elf`: ELF PowerPC de 32 bits, big-endian, sem strip.
- `cloud/dreamkai360_cloud_probe.elf32`: versão ELF32 PowerPC convertida e stripped.
- `cloud/ELF_AUDIT.txt` e `cloud/SHA256SUMS.txt`: auditoria e hashes.

[Abrir a execução e seus artefatos](https://github.com/MeiKinosaki/DreamKai360/actions/runs/38333640835).

### Como iniciar outra compilação

1. Abra a aba **Actions** deste repositório.
2. Selecione **DreamKai360 Cloud Xenon Toolchain**.
3. Clique em **Run workflow** e confirme.
4. Abra a execução para acompanhar as etapas e baixar os artefatos.

O cache do SDK pode acelerar execuções seguintes.

### Limitações importantes

- O programa `cloud/xenon_probe.c` é apenas um teste de compilador/linker; **não é o emulador Dreamcast**.
- O ELF produzido confirma que a toolchain consegue compilar e vincular um programa de teste PowerPC, mas não comprova execução no console Xbox 360.
- O código-fonte completo do emulador DreamKai360 ainda precisa ser integrado ao repositório. Portanto, **não existe ainda um `default.xex` do emulador**.
