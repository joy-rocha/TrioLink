BBPATH .= ":${LAYERDIR}" = basicamente ele permite o bitbake encontre arquivos de configurações/classes fornecidos pela lyers.

BBFILES += "${LAYERDIR}/recipes-\*/\*/\*.bb \
			${LAYERDIR}/recipes-\*/\*/\*.bbappend"

= o bbfiles basicamente diz para o bitbake procurar dentro da layer informada sobre as pastas correspondendo como as recipes .bb e .bbappend

BBFILE_COLLECTIONS += "nome_da_layer" = isso daqui é a identificação da layer

BBFILE_PATTERN_nome_da_tua_layer = "^${LAYERDIR}/" = diz que os arquivos começam no diretório principal, o root da tua layer

A prioridade basicamente informa a concorrência de threads e processos, besteira.

LAYERDEPENDS_nome_da_tua_layer = "core" = basicamente é de onde a layer ela é originada, como é do yocto/openembdded, por isso é do core

LAYERSERIES_COMPAT_nome_da_tua_layer = "scarthgap", diz que a layer é compactível com algum tipo, nesse caso, o Scarthgap.