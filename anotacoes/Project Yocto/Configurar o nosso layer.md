Lembre-se de sempre entrar no ambiente da build com o comando:
`source oe-init-build-env build-raspberry`. Caso de tu errar o comando faça isso:
```
unset BITBAKEDIR
`echo "$BITBAKEDIR"` = deve imprimir nada
```

Faz essa cronologia aqui para modificar a 'MACHINE' (é o teu _hardware_ que vai ser usado) -> cria a layer com o comando `bitbake-layers create-layer digita aqui o nome da tua layer`, não se esquece que ele vai criar no diretório em que ele estiver apontando, quer dizer, o ideal para criar no diretório que você está presente logo o nome da sua layer deve ser `../nomedalayer`