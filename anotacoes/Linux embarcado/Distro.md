https://docs.yoctoproject.org/dev/ref-manual/system-requirements.html

As distros que o Project Yocto recomenda que possuem suporte a compilação do Linux Embarcado como também as versões necessárias para que o Project Yocto rode normalmente

Geralmente precisa-se para compilar e mexer no linux embarcado (no caso na máquina host) de:
32 GB de RAM (16 já tá bom)
CPU de 4 núcleos

# Quais são os passo a passo iniciais para buildar:
https://docs.yoctoproject.org/dev/brief-yoctoprojectqs/index.html

Packages para baixar:
```sudo apt-get install build-essential chrpath cpio debianutils diffstat file gawk gcc git iputils-ping libacl1 libcrypt-dev locales python3 python3-git python3-jinja2 python3-pexpect python3-pip python3-subunit socat texinfo unzip wget xz-utils zstd

locale --all-locales | grep en_US.utf8
```

## instalar o python como ambiente virtual para o build do host
```
python3 -m venv --clear ./bitbake-setup-venv
. ./bitbake-setup-venv/bin/activate
pip install bitbake-setup
```

## Inicialização do _bitbake_
depois disso execute o comando:
`bitbake-setup init`

se quiser invés de iniciar em um diretório acima do diretório que está atualmente você então executa:
`bitbake-setup settings set --global default top-dir-prefix $HOME`

Esse comando aqui ele da um informe geral dos comandos a respeito do setup do bitbake para se configurar melhor.
`bitbake-setup settings list`

Depois disso vai aparecer um monte de coisa para configurar, por default vamos fazer o seguinte:
(Escolha de template de configuração) Poky-master -> (Aí dependendo da escolha anterior ele vai dar uma lista de configurações específicas) Poky -> (esse é o nosso "target") machine/qemux86-64  -> ai aqui tu da ENTER para o nome do diretório responsável por isso tudo, ou tu pode alterar o nome para colocar do jeito que quer

As configurações restantes vai ser baseado no exemplo acima

## Buildando a Image