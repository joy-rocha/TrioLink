Mostra todos os caracteres que da para usar com o compose
**`cat /usr/share/X11/locale/en_US.UTF-8/Compose | less`**






# Pacotes Arch

sudo pacman -Syu = atualiza o sistema

sudo make install = compila e faz com que o pacote se torne nativo no S.O.

sudo pacman -S [nome-do-pacote] =  instala um novo programa

sudo pacman -Rns [nome-do-pacote] = remove um programa e suas dependencias

pacman -Ss [termo] = pesquisa por um pacote nos repositorios oficiais

pacman -Qs [termo] = pesquisa por um pacote que ja instalado no computador

pacman -Qe = lista todos os pacotes instalador por mim

sudo pacman -Sc = limpa todo o cache de pacotes antigos baixados

yay -S [nome-do-pacote] = instala pacotes do AUR

## Wallpaper

yay -S mpvpaper

mpvpaper -l

mpvpaper eDP-1 /caminho/para/seu/video.mp4

nano ~/.config/autostart/wallpaper.desktop:
Cola isso aqui lah dentro: 
```
[Desktop Entry]
Type=Application
Exec=mpvpaper '*' /home/seu-usuario/caminho/para/seu-video.mp4 -o "loop"
Hidden=false
NoDisplay=false
X-GNOME-Autostart-enabled=true
Name=WallpaperAnimado
```

## Servicos

sudo systemctl start [servico] = inicia um servico

sudo systemctl stop [servico] = para um servico

sudo systemctl enable [servico] = faz um servico iniciar automaticamente quando o PC ligar

sudo systemctl disable [servico] = impede o servico de iniciar com o PC

sudo systemctl status [servico] = verifica se o servico estah rodando e se ha erros


# Comuns

pwd = mostra o caminho completo da pasta onde voce esta

ls = lista todos os arquivos no diretorio atual

ls -la = lista todos os arquivos, inluindo os ocultos, com permissao, tamanho e data

cd [nome-da-pasta] = entra em uma pasta especifica

cd .. = volta uma pasta para tras

cd ~ = volta para a home

mkdir [nome] = cria uma nova pata

rm [arquivo] = deleta um arquivo

rm -rf [pasta] = deleta uma pasta e tudo que tem dentro dela a forca

cp [arquivo-origem] [destino] = copia um arquivo

cp -r [pasta-origem] [destino] = copia uma pasta inteira

mv [origem] [destino] = move um arquivo/pasta de lugar, tambem renomeia arquivos

touch [nome-do-arquivo] = cria um arquivo de texto vazio

cat [arquivo] = le e exibe todo o conteudo de um arquivo de texto no terminal

nano [arquivo] ou vim [arquivo] = abre editores de texto no terminal para você modificar arquivos

TAB = autocomplete

Ctrl + C = cancela e interrompe o comando que esta sendo rodado no momento

Ctrl + L = limpa o terimnal

Ctrl + R = abre uma busca no seu historico de comandos, comecando a digitar ele acha o ultimo comando similar usado

Setas = navegam pelo historico de comandos

## Permissoes

sudo [comando] = privilegio de admin

su = troca de usuario para o root (exige senha de root)

chmod +x [arquivo] = torna um arquivo executavel eh necessario para rodar scripts

chown [usuario]: [grupo] [arquivo] = mudar o dono do arquivo

passwd = muda a minha senha de usuario

# Downloads web

wget [URL-do-arquivo] = baixa um arquivo da internet e salva onde estou

wget -c [URL] = continua um download que foi interrompido

curl -O [URL] = outra ferramenta para baixar arquivos

git clone [URL-do-repositorio] = baixa o codigo-fonte de um projeto do github ou gitlab

# Monitoramento do sistema 

top = mostra os processos em tempo real pela CPU e RAM

htop (ou btop)  = versoes mais bonitas de ler, eh igual ao top (precisa instalar pelo pacman -S htop)

ps aux = tira uma foto de todos os processos que estao sendo rodados no momento

kill [PID] = forca o fechamento de um programa usando o ID do processo, acha no htop

killall [nome do programa] = fecha todas as instancia de um programa pelo nome

free -h = mostra o uso da memoria RAM e swap em MB/GB

df -h = mostra o espaco livre e usado no seu HDD/SSD

du -sh [pasta] = calcula o tamanho da pasta especifica

neofetch (ou fastfetch) = exibe um resumo do sistema com a logo do Arch

pacman -Qg = mostra todos os pacotes instalados no sistema

pacman -Qg | grep cosmic = filtra os pacotes instalados pelo enviroment do cosmic

pacman -Q = lista todos os apps instalados

pacman -Q | less = com o **less** ajuda a navegar com as setas

### Bateria

Baixa os pacotes -> sudo pacman -S upower acpi

upower -e = verifica minha bateria

cat /sys/class/power_supply/BAT1/uevent = roda para ver se o driver da bateria ta funcionando, se nao for troca o **BAT1** por **BAT0**

dmesg | grep -i acpi = verifica se tem erros como **ACPI Error ou AE_NOT_FOUND**

acpi -V = se retornar **No support for device** executa logo em seguida esse -> lsmod | grep battery e se nao aparecer nada -> sudo modprobe battery -> upower -i

## Hardware
uname -r = mostra a versao do kernel linux

lsblk = lista todos os HDs, SSDS, pendrives e particoes

lscpu = mostra informacoes detalhadas do processador

lspci = lista os dispositivos conectados a placa mae

lsusb = lista todos os dispositivos conectados nas portas USB

## Extracao e compactacao de arquivos

tar -czvf [nome.tar.gz] [pasta] = pacta uma pasta no formato tar.gz

tar -xzvf [arquivo.tar.gz] = extrai um arquivo tar.gz

unzip [arquivo.zip] = extrai arquivos.zip (precisa instalar o pacote unzip)



# Redes e internet

ping [site.com] = mostra os pacotes para um site para ver se voce tem internet e medir a latencia, Ctrl+C para isso

ip a = mostra todos os seus enderecos de IP (local e interfaces de rede (cabo e wif-fi))

curl ifconfig.me = mostra o seu IP publico na internet

ss -tulpn = lista todas as portas abertas e conexoes de rede ativas

nmcli device wifi list = lista as redes wi-fi disponiveis ao redor

nmcli device wifi connect "[nome da rede]" password "[Senha]" = conecta a rede wifi pelo terminal

ip route = mostra todas as rotas de rede incluido o IP do seu roteador

