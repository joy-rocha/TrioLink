
bblayers.conf $\neq$ local.conf

**bblayers.conf**: seria os layers/metadados que o _Bitbake_ deve reconhecer. Geralmente tu coloca o pathing onde está localizado cada layer para o bitbake.

**local.conf**: para qual máquina vamos construir, dependências que vão vir instalados na imagem, o que habilitar

**OBS.:** Importante é de que o .bbappend vem com uma % antes, exemplo, como meio de dizer que não importando a versão, ela vai funcionar tlgd.
```
linux-raspberrypi_%.bbappend
```