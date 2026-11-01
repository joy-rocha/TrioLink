Só possui suporte para Debian, Fedora, AlmaLinux, OpenSUSE e Rocky Linux. *Presta atenção abaixo nas versões dos S.O*. Respectivamente:
* AlmaLinux: Versões 8 e 9.
* CentOS Stream: Versões 9 e 10.
* Debian: Versões 11, 12 e 13.
* Fedora: Versões 42 e 43.
* openSUSE Leap: Versões 15.6 e 16.0.
* Rocky Linux: Versões 8 e 9.
* Ubuntu: Versões 22.04 LTS, 24.04 LTS, 25.04, 25.10 e 26.04 LTS.

A maior parte desses comandos iniciais são aqueles que estão tanto disponíveis e orientados pela Yocto e como estamos trabalhando especificamente com a Raspberry Pi, com a devida orientação da própria rasp.

Os pacotes para instalar o _bitbake_ e outras ferramentas a mais tem que pesquisa, tudo vai ser baseado no Ubuntu aqui mesmo. Os pacotes iniciais para instalar no *UBUNTU*:

Da um update no teu sistema operacional:
```shell
sudo apt update
```

## Instalando dependências

Instala as dependências dos pacotes respectivamente para ter o ambiente Yocto:
```shell
sudo apt install gawk wget git diffstat unzip texinfo gcc-multilib \
build-essential chrpath socat cpio python3 python3-pip python3-pexpect \
xz-utils debianutils iputils-ping python3-git python3-jinja2 libegl1 \
libsdl1.2-dev pylint xterm python3-subunit mesa-common-dev zstd \
liblz4-tool
```

(É de fato, necessário que se tenha conhecimentos a respeito de programação em shell e noções básicas de comandos de Linux. O ambiente Yocto é extremamente organizado, porém gigantesco e acabar se perdendo dentro dos repositórios quando se baixar as extensões)

Após a instalação de dependências vamos de fato clonar o ambiente yocto usando os seguintes comandos, claro utilizando o scarthgap, vamos:
```shell
git clone -b scarthgap https://git.yoctoproject.org/git/poky #(ambiente poky)

#(Logo em seguida:)

cd poky
git clone -b scarthgap https://git.yoctoproject.org/git/meta-raspberrypi #(baixa a layer do raspberrypi)
```

## Preparando o ambiente

Logo em seguida, dentro do ambiente poky, você vai executar a seguinte linha de comando:
```shell
source oe-init-build-env build-raspberry #inicializa o ambiente para especificamente raspberry
```
Se caso venha fechar o vscode, é necessário sempre executar esse comando antes para o _BitBake_ poder entender que se trata de build e "cozinhar" as _recipes_.
`oe-init-build-env` = é o script para inicializar o ambiente
`build-raspberry` = é o tipo de ambiente que queremos poderia ser um nome qualquer, quer dizer, invés de build-raspberry, poderia ser build projeto assert.

Essa build é responsável pela conexão dos diferentes tipos de layer e principalmente quando for criar um nosso e o bitbake precisar buildar ele reconhecer o nosso layer e as modificações que fizermos nele. Não só isso, ser possível dentro do nosso shell os comandos do bitbake como bitbake-layers,etc.

Para ter certeza que foi pode executar esses 2 comandos para conferir, mas geralmente ele cospe uma resposta no terminal, se deu erro, provavelmente erro seu mesmo que digitou errado.
```shell
ls -ld bitbake # vai devolver algo como drwxr-xr-x ... bitbake
ls -l oe-init-build-env
```

E se ainda persistir o erro é porque a primeira build causou esse erro, pra consertar executa isso aqui:
```shell
unset BITBAKEDIR
echo "$BITBAKEDIR" #verifica se realmente tem nada
```
### fazendo o bitbake reconhecer a layer

```shell
bitbake-layers add-layer ../meta-raspberrypi #ele diz aqui que existe uma layer chamada meta-raspberrypi, logo adicione aqui suas configurações e recipes e considere-as, se for outro hardware, adicione ele respectivamente.
bitbake-layer show-layers # mostra as layers disponíveis baixadas
```
Pra verificar após criar a layer tu vai fazer o seguinte:
```shell
which bitbake 
which bitbake-layers
#mostram o caminho absoluto onde se encontra ambos para ver se realmente estão lá
ls -l ../bitbake/bin/bitbake* # para mostrar os executáveis e scripts disponíveis

```

Se os dois primeiros comandos de which não mostrar nada, o path tá errado então tu executa esses 4 comandos aqui, eles vão dizer sobre o que tá acontecendo, quer dizer, vão cuspir para tu apontando os caminhos se estão verdadeiramente corretos.
```shell
echo "$BITBAKEDIR" #é para mostrar nada
echo "$BUILDDIR" #é para mostrar nada
echo "$PATH" #ele vai dizer o local do bitbake, besteira
echo "$BBPATH" # é para mostrar nada
```
Esse comando aqui vai dizer exatamente o que o bitbake está fazendo ao tentar executar o comando
```Bash
../bitbake/bin/bitbake-layers --help #ele vai devolver o passo a passo de execução do bitbake e vai dizer o erro. O erro que encontrei, por exemplo, era de que não tinha o arquivo 'en_US.UTF-8'
```


#### Problema do arquivo 'en_US.UTF-8'
```Bash
sudo locale-gen en_US.UTF-8 #vai gerar o arquivo necessário no teu ubuntu. Executa ele dentro da pasta build-raspberry, lembra inicializar ele antes caso não estiver dentro
```
Tu vai sair `cd ..` e logo em seguida vai dar
```bash
unset BITBAKEDIR
```
Aí sim os dois comandos de:
```bash
which bitbake
which bitbake-layers
```
irão funcionar.

## Alterando as configurações necessárias para apresentar o raspberrypi5

Vai ai e abre o conf/local.conf dentro do poky no build-raspberry (ou no caso a layer que tu tenha copiado) e altera a máquina para a que tu quiser aí que tu esteja planejando. No nosso caso é "raspberrypi5".

Após isso, vai na build-raspeberry, verifica ai qual será o formato que a image vai ser compilada quando fizermos absolutamente **TUDO**. O nome disso é `IMAGE_FSTYPES = "tar.bz2 ext3 wic.bz2 wic.bmap"`

Foi decidido em usar o kernel IIO do Linux, porém, para habilitar o I2C a seguinte linha deve ser escrita dentro do local.conf do build-raspeberry
```Python
#isso ai ativa o I2C
ENABLE_I2C = "1"
```

Para saber se compilou:
```bash
bitbake core-image-minimal
#para ser mais específico seria o
bitbake -e core-image-minimal | grep '^ENALBE_I2C='
```

## Criando nossa layer
```bash
bitbake-layers create-layer nome-da-tua-layer-aqui #ele gera nosso conf/layer.conf, algumas receitas iniciais, readme e tals
bitbake-layers add-layers nome-da-tua-layer #necessário para o reconhecimento da layer pelo bitbake
```
A layer deve ser criada dentro do diretório do poky viu! O que é de extrema importância seria o conf/layer.conf, ele é quem faz o bitbake ser reconhecer.

O ideal é que para cada recipe nova que for criada, quer dizer .bb e .bbappend específicos, o ideal é ter uma divisão adequada dentro da layer para não misturar as coisas, quer dizer, recipes que se tratam do kernel, recipes de suporte (recipes externas que foram necessárias serem baixas), recipes-core que são as principais e assim vai.
## Kernel da layer que vamos usar
Para descobrir o kernel a seguinte sequência de comandos foi usada:

```Bash
bitbake -e virtual/kernel | grep '^PV=' #ele retorna a versão do kernel selecionado pelo bitbake obtido pelo repositório do git
bitbake -e virtual/kernel | grep '^PREFERRED_PROVIDER_virtual/kernel=' #ele diz a recipe usada para o kernel
bitbake-layers show-recipes nome_que_tu_achou_do_comando_anterior_a_este #vai mostrar as recipes de kernel proveniente da que a gente encontrou. ele basicamente só da retornou nos .bb que tu mostrou dos .git que estão sendo usados anteriormente a este comando

```

Para o nosso caso, precisamos agora criar um diretório específico que vai dizer ao linux, seria no caso:
```Bash
mkdir -p ..nome_da_layer_que_tu_colocou/recipes-kernel/linux
```
Dentro do diretório tu vai criar um arquivo com a seguinte extensão: 
```Bash
.bbappend
```
Foi dito aqui sobre a questão da % [[Diferença de configurações da layer]], logo antes da extensão coloca-se:
```bash
_%.bbappend
```

Dentro da tua layer tu vai executar isso, lembre sempre de estar dentro do ambiente do bitbake.
```bash
bitbake-layers show-appends nome_do_kernel_linux_encontrado 
```

## Verificações se está tudo em ordem

Como estamos usando o meta-raspberrpi, vamos considerar a seguinte linha de sequência, se caso usar outro hardware, lembre de modificar o diretório correspondente.
```bash
grep -nE 'KERNEL_CONFIG|SRC_URI|FILESEXTRAPATHS' \
../../meta-raspberrypi/recipes-kernel/linux/linux-raspberrypi_6.6.bb #deve retornar algo como 10:SRC_URI ="\
sed -n '1,35p' ../../meta-raspberrypi/recipes-kernel/linux/linux-raspberrypi_6.6.bb #isso mostra a estrutura da receita, vamos focar em quais fragmentos vamos pegar
```

Após isso da para entrar assim:
```bash
bitbake -c menuconfig virtual/kernel #ele vai abrir o kernel de maneira virtual, descobrir sensores, atuadores, etc. Procura ai o que tu quer
```

### Adicionando os periféricos presentes do kernel

Para ficar organizado daquilo que vamos usar do kernel, o ideal é estabelecermos um diretório próprio para isso, obviamente ele deve ficar dentro da nossa layer, especificamente dentro do outro diretório que criamos anteriormente com caminho `../recipes-kernel/linux/` . Após a criação dele, vamos adicionar um arquivo config de kernel, com extensão .cfg, no nosso caso, vamos usar o sensor bmp280. Logo
```bash
code meta-triolink/recipes-kernel/linux/files/triolink-bmp280.cfg
```
Dentro dele vamos habilitar baseado no [[Como funciona dentro do kernel?]]. 
```
CONFIG_BMP280=m
```
Agora abre lá o .bbappend que puxamos do kernel linux:
```bash
FILESEXTRAPATHS:prepend := "${THISDIR}/files:" #essa liha basicamente diz para procurar arquivos locais que ficam no diretório que criamos anteriormente, que fica ao lado do .bbappend.

SRC_URI += "file://triolink-bmp280.cfg"
#esse arquivo faz parte dos arquivos utilizados pela receita

KCONFIG_FRAGMENTS += "triolink-bmp280.cfg" #esse arquivo vai ser usado como fragmento de kernel


do_configure:append() {
    install -m 0644 ${WORKDIR}/triolink-bmp280-overlay.dts \
        ${S}/arch/arm64/boot/dts/overlays/

    install -m 0644 ${WORKDIR}/triolink-mpu6500-overlay.dts \
        ${S}/arch/arm64/boot/dts/overlays/
}

# garante inicialização no boot
RPI_KERNEL_DEVICETREE_OVERLAYS:append = " overlays/triolink-bmp280.dtbo"
RPI_KERNEL_DEVICETREE_OVERLAYS:append = " overlays/triolink-mpu6500.dtbo"
```
Caso de tu ter feito uma cagada, remove o cache temporáriod e tmp:


```bash
rm -rf tmp
```

Depois disso executa de novo o comando de entrar no kernel virtual para ver se não possui nenhum erro e está tudo ok.

Sobre a questão da overlay, tu vai ter que pesquisar, mas vamos assumir que seu _hardware_ possui esse overlay necessário. Para verificar qual barramento de endereço seu periférico é ativado pelo i2c, tu vai ai instala rapidamente algum S.O. pré definido e baixa o i2cdetect e coloca esse comando:
``` bash
i2cdetect -y 1
```

Ele vai mostrar qual barramento o sensor está que o rasp manda para se comunicar com ele por registrador. Precisamos agora criar um arquivo dts (device tree), ele é o responsável para fazer o código do rasp se comunicar com o periférico, nesse caso vamos fazer um arquivo com a extensão .dts, exemplo:
```bash
triolink-bmp280-overlay.dts
```
Dentro do .bbappend você coloca a seguinte linha:
```bash
RPI_KERNEL_DEVICETREE_OVERLAYS:append = " overlays/triolink-bmp280.dtbo"
```

Aí agora é pura pesquisa e entendimento do que o terminal vai cuspir, para montar um arquivo .dts tem que estar ciente das versões e ter exatamente os nomes escritos como fonte. Para verificação a seguinte sequência de comandos pode ser usada:
```bash
bitbake-layers show-recipes linux-raspberrypi #confirmação da nossa recipe
bitbake-layers show-appends linux-raspberrypi
```

Para descobrir o código-fonte do kernel:
```bash
ls -la tmp/work-shared/raspberrypi5/kernel-source/
#os 3 comandos abaixo em seguida de git ele vai disponibilizar as versões do kernel
git -C tmp/work-shared/raspberrypi5/kernel-source describe --tags --always

git -C tmp/work-shared/raspberrypi5/kernel-source branch --show current

git -C tmp/work-shared/raspberrypi5 tmp/work-shared/raspberrypi5/kernel-source log -1 --oneline

find tmp/work-shared/raspberrypi5/kernel-source/arch/arm64/boot/dts \
    -type f | grep -E 'bcm2712|rpi-5' #ele vai devolver algo como: bcm2712*.dtsi bcm2712*.dts
...

grep -RniE 'i2c[0-9]?|i2c@' \
    tmp/work-shared/raspberrypi5/kernel-source/arch/arm64/boot/dts/broadcom/ \
    | grep -E 'bcm2712|rpi-5' #aponta para o controlador correto do i2c
```

Para descobrir exemplo de como o kernel estrutura o overlay:
```bash
find tmp/work-shared/raspberrypi5/kernel-source/arch/arm64/boot/dts/overlays \
    -maxdepth 1 -type f | head -30
```

Como ficou organizado o que eu usei o bmp280 (é muito mais simples esse trabalho massante ser feito pela IA):
```bash
/dts-v1/;
/plugin/;

/ {
    compatible = "brcm,bcm2835";

    fragment@0 {
        target = <&i2c_arm>;
        __overlay__ {
            status = "okay";

            bmp280: bmp280@76 {
                compatible = "bosch,bmp280";
                reg = <0x76>;
                status = "okay";
            };
        };
    };
};
```

Existe um problema quando vamos tratar mais de um periférico através do kernel, por sua vez, ele vai acabar por dar erro no makefile devido a como ele gera os .dtbo, temos 2 opções de maneira **manual ou automática**. 
#### Manual
Logo, precisamos consertar isso gerando o seguinte arquivo.
```bash
nano 0001-triolink-add-dt-overlays.patch
```

Dentro dele vai ter o seguinte conteúdo:
```bash
From 0000000000000000000000000000000000000000 Mon Sep 17 00:00:00 2001
From: TrioLink <triolink@localhost>
Date: Tue, 30 Sep 2026 00:00:00 +0000
Subject: [PATCH] overlays: add TrioLink BMP280 and MPU6500

Add the custom Device Tree overlays used by the TrioLink project.

Signed-off-by: TrioLink <triolink@localhost>
---
 arch/arm64/boot/dts/overlays/Makefile | 2 ++
 1 file changed, 2 insertions(+)

diff --git a/arch/arm64/boot/dts/overlays/Makefile b/arch/arm64/boot/dts/overlays/Makefile
index 000000000000..111111111111 100644
--- a/arch/arm64/boot/dts/overlays/Makefile
+++ b/arch/arm64/boot/dts/overlays/Makefile
@@ -3,6 +3,8 @@
 dtb-$(CONFIG_ARCH_BCM2835) += overlay_map.dtb hat_map.dtb

 dtbo-$(CONFIG_ARCH_BCM2835) += \
        triolink-bmp280.dtbo \
        triolink-mpu6500.dtbo \
        act-led.dtbo \
        adafruit-st7735r.dtbo \
        adafruit18.dtbo \
-- 
2.39.2
```

#### Automática
Entre dentro do kernel-source, mas ai tu vai ter que descobrir o caminho. Eu descobri o do raspberrypi5 que seria:
```bash
cd ~/Documentos/Luis/Distro_TrioLINK/poky/build-raspberry/tmp/work-shared/raspberrypi5/kernel-source
```

Depois abri o makefile sabendo o caminho absoluto dele:
```bash
nano arch/arm64/boot/dts/overlays/Makefile
```

Dentro do arquivo altera-se a seguinte linha correspondente da geração dos .dtbo, porém, especificamente, como estou tratando dos meus sensores logos as linhas adicionadas são essas:
```bash
#apos essa
dtbo-$(CONFIG_ARCH_BCM2835) += \

#adiciona essas ai abaixo
triolink-bmp280.dtbo \
triolink-mpu6500.dtbo \
```

## Retirando a restrição de licença

Por algum motivo, é necessário adicionar essa linha no local.conf

``` Python
LICENSE_FLAGS_ACCEPTED += "synaptics-killswitch"
```

## Descobrir extensões

Pra tu instalar outras extensões que tem de fora, como se fosse um sudo apt e tals de forma que toda imagem compilada traga esses arquivos já dentro da build (sem precisar depender de instalar eles toda vez) você vai ter que procurar manualmente o nome de cada um e adicionar, as vezes os nomes que tem para instalar sei lá pelo ubuntu é totalmente diferente daqueles que estão disponibilizados pelo yocto. Para procurar isso em específico usa-se esse comando:
``` bash
bitbake-layers show-recipes nome_da_extensão_que_quer_buscar
```

Se não tiver, vai ter que baixar por fora. Exemplo de uma recipe que não está dentro do project Yocto é a biblioteca do lgpio, logo, criamos um .bb inicialmente contendo o seguinte, resalto que o uso de IA facilita mais sua vida, mas se quiser fazer tudo na mão, te vira:
```python
SUMMARY = "Linux GPIO library"
DESCRIPTION = "C library for controlling GPIOs on Linux SBCs"
HOMEPAGE = "https://github.com/joan2937/lg"
SECTION = "libs"

LICENSE = "Unlicense"
LIC_FILES_CHKSUM = "file://UNLICENCE;md5=61287f92700ec1bdf13bc86d8228cd13"

#ele baixa o repositório oficial que queremos
SRC_URI = "git://github.com/joan2937/lg.git;protocol=https;branch=master"

#diz a tag de versão que queremos, no caso a do repositório oficialmente é literalmente esta abaixo
SRCREV = "v0.2.2"

#ele transfere o código depois de clonar para esse local aí  para faciltitar para a gente
S = "${WORKDIR}/git"

#a linha abaixo faz o Makefile receber as ferramentas corretas do Yocto
EXTRA_OEMAKE = "CC='${CC}' AR='${AR}' RANLIB='${RANLIB}' STRIP='${STRIP}'"

#necessária para evitar erros
INSANE_SKIP:${PN} += "already-stripped"

#ele diz ao yocto para entrar no diretório do código e executar o makefile usando o ambiente de compilação do Yocto
do_compile() {
    oe_runmake
}

#isso faz instalar os pacotes necessários da biblioteca do lgpio
do_install() {
    install -d ${D}${libdir}
    install -d ${D}${includedir}

    install -m 0755 ${S}/liblgpio.so.1 ${D}${libdir}/liblgpio.so.1
    ln -sf liblgpio.so.1 ${D}${libdir}/liblgpio.so

    install -m 0644 ${S}/lgpio.h ${D}${includedir}/lgpio.h
}

FILES:${PN} += "${libdir}/liblgpio.so.1"
FILES:${PN}-dev += "${includedir}/lgpio.h ${libdir}/liblgpio.so"
```

Após a criação dessa recipe ela precisa ser adicionada manualmente dentro do local.conf.


## Como colocar a imagem dentro do armazenamento para transferir para a rasp:

O programa a ser utilizado oficialmente seria o `bmaptool` que serve para "flashar images", logo o comando para baixar ele:
```bash
sudo apt install bmap-tools
```
E para identificar a unidade de armazenamento responsável para ser responsável e substituido do comando de "copy" seria esse:
```
lsblk
```
A seguinte sequência deve ser seguida para melhor aproveitamento de colocar a imagem, neste caso, estou usando o pendrive, se for outro espaço de armazenamento é aconselhável procurar antes sobre:
```
# Verificar o .wic.bz2:
ls -lh tmp/deploy/images/raspberrypi5/*.wic*

# Empacotar:
cp --reflink=never tmp/deploy/images/raspberrypi5/aqui_tu_subtitui_do_que_tu_achou_do_comando_anterior_e_geralmente_ele_é_o_último_que_aparece /tmp/triolink.wic.bz2

# Verificar se chegou mesmo:
ls -lh /tmp/triolink.wic.bz2

# Desempacotar:
rm /tmp/triolink.wic
bunzip2 -k /tmp/triolink.wic.bz2

# Enviar para unidade de armazenamento, (lembre de substituir /dev/sdX pela unidade de armazenamento correspondente):
sudo bmaptool copy --nobmap /tmp/triolink.wic /dev/sdX

```

## Diagnóstico de erro de transferência de arquivos

Foi descoberto um erro que gerou dor de cabeça durante a compilação de uma imagem nova, o Yocto por padrão coloca para o boot ser detectado sempre pela porta do cartão MicroSD, logo, se o usuário estiver usando outro tipo de unidade de armazenamento, o Yocto não vai saber, então, toda vez que copiar uma nova imagem é necessário alterar o cmdline.txt na linha especificamente onde ele aponta o root, `root = seila`. Essa linha e responsável pelo boot da unidade de armazenamento, sendo assim, dentro do local.conf, adicione a seguinte linha referente a boot de pendrives:
```python

```

## Ferramenta de dev

O Yocto ele possui uma ferramenta que basicamente é um super do super usuário (aquele em que você precisa as vezes conceder permissão temporária através do sudo) que seria o root, para remover essa ferramenta de debugg basta procurar no local.conf especificamente a linha:
```python
EXTRA_IMAGE_FEATURES ?= "debug-tweaks"
```
alterar para:
```python
EXTRA_IMAGE_FEATURES ?= ""
```