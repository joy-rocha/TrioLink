
Symbol = seria o símbolo correspondente ao sensor
Type: tristate (significa que ele possui 3 estados e estão apresentados):
```c
[ ] //desabilitado
[M] //módulo, o driver é grado como um módulo .ko e carregado pelo linux quando necessário
[*] //embutido diretamente no kernel, fica dentro do kernel.img e é carregado junto com o kernel
```

Se no kernel apresentar a seguinte texto:
```
Depends on: IIO [=n] && (I2c [=y] || SPI_MASTER [=y])
```
traduzindo, dis que precisa ter suporte I2C ativado ou SPI, e o kernel precisa ter o IIO habilidade. O **n** significa que ele não está habilitado no momento.