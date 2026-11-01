Para iniciar o _enviroment_ da Yocto, toda vez que for modificar ou compilar a distro tu vai executar esse comando aqui:
`source oe-init-build-env build-raspberry`
`which bitbake` = verifica se está funcionando o bitbake, se retornar nada, significa que não está logado

## gráfico

Pra criar um gráfico das dependências de layers tu pode dar um "-g"

# verificar: 

`grep '^MACHINE' conf/local.conf` = verifica a machine no código (é a máquina no qual o código vai compilar, digamos assim, é o layer responsável por definir qual o tipo de _hardware_)
`bitbake -e core-image-minimal | grep '^aqui digita o que você quer verificar' `
^PV = verifica a versão do kernel

**Se uma recipe não estiver funcionando você usa o comando "Bitbake foo", foo é o nome específico daquela recipe**
# layers

`bitbake-layers create-layer meta-triolink` = cria uma layer
`bitbake-layers show-layers` = mostra todas as layers dentro da distro
`bitbake-layers add-layer nome_da_layer` = adiciona uma layers para o bitbake
`find nome_da_layer -maxdepth 3 -type f` = mostra os arquivos dentro da layer


Uso comum -> _recipename_ - basicamente é o nome da _recipe_ em que vai _buildar_ (referenciada como "alvo"), com o intuito de processar esse _recipe file_ voce digita:

bitbake matchbox-desktop

comandos:
bitbake -h
bitbake --help

# kernel
`bitbake -c menuconfig virtual/kernel` = ele vai baixar, caso não tenha e vai abrir o kernel do linux responsável pelo _hardware_ do que tu tá mexendo


Cria um arquivo .cfg dentro da sua meta-layer em seguida para ficar melhor a visualização cria recipes-kernel/linux -> files -> para adicionar um dos modulos do kernel escreve dentro da pasta CONFIG_NOMEDOMODULO

# buildar

`bitbake core-image-minimal`