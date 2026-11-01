**_Append Files_**: conhecidos geralmente como `bb.append` files que contém a informação da build da _recipe_. O _append file_ e a _recipe file_ devem usar o mesmo _root filename_. OBS.: O uso do % é limitado e é usado exatamente na posição que está para se referir as porções dos _recipes_ que pertencem ao _append file_  Ex.:
```
Append file é esse = busybox_1.21.%.bbappend

as recipes desse Append file segue seu nome logo:
busybox_1.21.1.bb
busybox_1.21.2.bb
busybox_1.21.3.bb
busybox_1.21.10.bb
busybox_1.21.25.bb
```

**_Configuration files_**: são os arquivos responsáveis pelas variáveis e as variáveis do usuário e a configuração de informação do _hardware_. Esses arquivos basicamente fazem -> "O que construir e o que colocar na imagem para que ela tenha suporte." Terminam geralmente com _.conf_ de extensão. O _conf/local.conf_ contém a definição de variáveis pelo usuário que afetam quaisquer _build_. O `meta-poky/conf/distro/poky.conf` define a distro do Yocto na onfiguração de variáveis e uso somente quando estiver _buildando_. Ex.: `machine/beaglebone.conf` que defiem a configuração de variáveis de Texas Instruments ARM

_**Configuration Data:**_ define configurações específicas de máquina, decisões políticas e então adiante, é basicamente o que junta tudo.

_**Build Directory**_: é a área usada pela _OpenEmbedded_ como sistema das _builds_.

_**Build-in Fragment**_: serve basicamente fazer com que uma variável única seja afetada de forma global. Pode ser ativada/desativada pelo _Bitbake_, o seguinte comando pode ser feito:
`<fragment>:<variable name>` = possibilita a modificação do valor da variável em questão
`machine/qemux86-64` = ele seta a variável através do _fragment_ para a "machine" que vai estar ser o nosso linux embarcado.

_**Buildtools**_: é um binário que possui outras ferramentas de compilação/interpretação necessárias para fazer o Project Yocto rodar. TEm o extended e o make, o extended vem com ferramentas a mais e o make basicamente providencia versões requisitadas do make para fazer rodar o _OpenEmbedded_ sistema de build.

**_Payload_** - o conteúdo dentro de um protocolo de comunicação. nome das variáveis, estrutura dos campos, tipos dos valores e significado dos valores fazem parte do schema do payload. Schema = são as regras rigidas, payload = é a mensagem montada baseada no Schema.I2C - procolo de comunicação do Raspeberry Pi 5 aos dispositivos eletrônicos

driver de kernel/IIO - padronização do kernel do linux para a comunicação entre sensores e conversores de dados

I2C / Driver de kernel/IIO - principal diferença é que o I2C trata de uma linguagem de baixo nível, mas sem tanta retrocompatibilidade e o Driver de kernel/IIO já é uma linguagem de alto nível de comunicação os drivers do kernel com o próprio sistema.

Image - conjunto de software que será colocado no armazenamento do Raspberry Pi para que ele consiga inicializar e funcionar como o sistema Linux. Pacote completo do sistema Linux

Recipes $\neq$ Drive - descrição de como o componente, software, etc. Entra dentro do sistema e está localizado

**_Layer_**: incluem _metadata_. Conseguem sobrescrever especificações anteriores. É possível customizar sua própria *Layer* adicionado suas "camadas (as layers)" depois da que você se inspirou.

**_Metadata_**:  incluem as _Recipes_, os _Configuration files_, _Distro configuration files_ e/ou _class files_ - _class files_ são basicamente informações comuns abstratas de _buildar_, ex.: como construir um kernel -, (como também outras informações que fazem referência a si mesma durante a _build_ e também quais os dados são feitos para controlar as coisas que são buildadas e os efeitos da _build_).


**_Recipes_**: a forma comum de _**metadata**_, contém uma lista de configurações e instruções para buildar os **_packages_**. Ex.: instruções.

**_Board Support Package_** - essa camada aqui é de abstração para os sistemas embarcados e serve para listar para o S.O. onde está localizado cada coisa.

**_BSP (Board Support Package)_** do meta-raspberrypi (mapa de _hardware_) = é na verdade uma camada de BSP que ajuda as distro linux que são feitas a entenderem e terem acesso mais conciso dentro do raspeberry, como se fosse uma camada de abstração extra. Essa camada aqui me diz como está disposta a minha _**Device Tree**_.
https://meta-raspberrypi.readthedocs.io/en/latest/

**_Device Tree_** - é onde o Linux precisa estar para saber sobre o _Hardware_, basicamente é o nome do arquivo onde está localizada a _BSP_.

_**Classes**_: contém informações que podem ser úteis durante o compartilhamento entre _recipes files_. É arquivos que possuem encapsulamento lógica e hereditariedade que comumente utiliza-se para definir de vez e facilmente o multi uso de _Recipes_ 

**_Publisher_** - será quem publica os dados (será o raspberry)
**_Subscriber_** - é quem escreverá a mensagem.
_Topic_ - é o endereço lógico onde a mensagem está armazenada
**_Broker_** - intermerdiário que recebe e distribui as mensagens MQTT. O cara responsável pelo _Broker_ vai mandar a mensagem codificada para ele e enviar para a próxima etapa.

**Poky** - Faz parte do Project Yocto e é usado com o intuito de fornecer ferramentas adequadas ao dev para trabalhar com um projeto de sistemas embarcados, usa uma coisa chamada de "_Bitbake Tool_". Coda linha por linha. É uma **distribuição**.

**Ângström** - Vem já pré-compilado ao contrário do Poky. O Poky você cria tudo do 0 e o Ângström já vem com algumas coisas pré setadas. É uma **distribuição**.


**I2C** - procolo de comunicação do _Hardware_ aos dispositivos eletrônicos

**driver de kernel/IIO** - padronização do kernel do linux para a comunicação entre sensores e conversores de dados

**I2C / Driver de kernel/IIO**- principal diferença é que o I2C trata de uma linguagem de baixo nível, mas sem tanta retrocompatibilidade e o Driver de kernel/IIO já é uma linguagem de alto nível de comunicação os drivers do kernel com o próprio sistema.

**_Packages_**: são as **_Recipe's_ do Bitbaker**. São os binários compilados produzidos pelas fontes dos _Recipes_.

Image - conjunto de software que será colocado no armazenamento do Raspberry Pi para que ele consiga inicializar e funcionar como o sistema Linux. Pacote completo do sistema Linux

Recipes $\neq$ Drive - descrição de como o componente, software, etc. Entra dentro do sistema e está localizado

OBS.: Geralmente uma "_layer_" começa com a palavra meta-, não é obrigatório, mas é o que se indica.

Pesquisar:
https://docs.yoctoproject.org/dev/overview-manual/yp-intro.html?utm_source=chatgpt.com tópico 1.3.1. Estudar melhor o tópico 1.3.2
_VSCode IDE Extension_ = procurar ver
_devtool_
_Extensible Software Development Kit (eSDK)_
_Toaster_

## _Development Host_

É basicamente o S.O. inicial que tu vai estar compilando e codando, quer dizer, imagina que tu ta codando e compilando no Ubuntu, logo esse é o _software_ denominado _Development Host_.
## _OpenEmbedded Build System_: 

**_Bitbake_** é basicamente uma engine que analisa instruções (Ex.: recipes) e dados de configuração. Depois disso, ele cria uma árvore de dependência para ter uma: ordem de compilação, planejamento de compilação e execução da image Linux. A compilação cruzada é feita e montada na tentativas de criar uma ferramenta adequada.

## _OpenEmbedded-Core (OE-Core)_

São _metadatas_ que são _Recipes_ de fundação, classes e arquivos associados que são comuns em diferentes sistemas _OpenEmbedded_.
