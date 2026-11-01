
**INSTALL** - Para pacotes específicos, quando quiser incluir um programa, biblioteca ou ferramenta. Ex.:
```python
IMAGE_INSTALL:append = " nano"
IMAGE_INSTALL:append = " mosquitto-clients"
IMAGE_INSTALL:append = " cjson"
```

**FEATURES** - Habilita conjunto de funcionalidades, quando quiser ativar um recurso fornecido pelo Yocto em vez de especificar cada pacote individualmente
```python
IMAGE_FEATURES:append = " ssh-server-openssh"
```
