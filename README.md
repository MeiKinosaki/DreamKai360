# DreamKai360
Emulador de Dreamcast para Xbox 360.

## Toolchain Xenon na nuvem

O workflow **DreamKai360 Cloud Xenon Toolchain** está em `.github/workflows/xenon-cloud.yml`. Ele usa um runner temporário do GitHub Actions para instalar dependências, obter o builder upstream do Free60Project/libxenon, compilar a toolchain Xenon, compilar a biblioteca libXenon e tentar gerar um ELF de teste.

### Como iniciar

1. Abra a aba **Actions** deste repositório.
2. Selecione **DreamKai360 Cloud Xenon Toolchain**.
3. Clique em **Run workflow** e confirme em **Run workflow**.
4. Abra a execução mais recente para acompanhar as etapas e consultar os artefatos/logs.

A primeira compilação pode levar bastante tempo. Quando o cache do SDK for salvo com sucesso, execuções posteriores poderão reutilizá-lo.

### Limites atuais

- O arquivo `cloud/xenon_probe.c` é apenas um programa mínimo para testar o compilador/linker; **não é o emulador Dreamcast**.
- O workflow precisa concluir com sucesso para confirmar que a toolchain foi compilada. As primeiras execuções publicadas falharam e ainda precisam de diagnóstico; não há, por enquanto, um ELF validado para download.
- O código completo do emulador ainda precisa ser integrado a este repositório antes de compilar o DreamKai360 propriamente dito.
