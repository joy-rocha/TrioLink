# **LABORATÓRIO ASSERT**

**ASSERT CAPACITA**

*Normas de Desenvolvimento de Firmware Embarcado*

*Estilo, documentação e arquitetura*

#### **Laboratório Assert**

Telefone: +55 (83) 3612-1296 Email: contato@assert.ifpb.edu.br Homepage: www.assert.ifpb.edu.br

*Revisão deste documento: 1.0*
*Revisão deste documento: 1.1*

![](_page_0_Picture_7.jpeg)

![](_page_0_Picture_8.jpeg)

Autores: Gabriel Domingos de Medeiros Luiz Oliveira de Souza Neto

![](_page_1_Picture_1.jpeg)

# **SUMÁRIO**

| 1 - | - OBJETIVO                                                 | 3    |
|-----|------------------------------------------------------------|------|
| 2 - | - REGRAS DE ESTILO                                         | 4    |
|     | 2.1. Idioma                                                | 4    |
|     | 2.2. Encoding                                              | 4    |
|     | 2.3. Convenção de Nomenclatura — Dados                     | 4    |
|     | 2.4. Convenção de Nomenclatura — Arquivos                  | 4    |
|     | 2.5. Convenção de Nomenclatura — Funções                   | 5    |
|     | 2.6. Convenção de Nomenclatura — Typedefs                  | 5    |
|     | 2.7. Convenção de Nomenclatura — #defines                  | 6    |
|     | 2.8. Convenção de Nomenclatura — enums                     | 6    |
|     | 2.9. Indentação: geral                                     | 7    |
|     | 2.10. Indentação: funções, condições e loops               | 7    |
|     | 2.11. Indentação: especificamente condicionais             | 8    |
|     | 2.12. Indentação: diretivas de pré-compilação condicionais | 9    |
| 3 - | - REGRAS DE DOCUMENTAÇÃO                                   | .10  |
|     | 3.1. Formato de comentários                                | 10   |
|     | 3.2. Cabeçalhos de Arquivos                                | .11  |
|     | 3.3. Funções                                               | .12  |
|     | 3.4. Documentação de Configurações                         | . 13 |
| 4 - | - REGRAS DE ARQUITETURA                                    | .15  |
|     | 4.1. Filosofia                                             | . 15 |
|     | 4.2. Seções de Código                                      | . 15 |
|     | 4.3. Includes                                              | . 17 |
|     | 4.4. Defines locais vs. defines públicos                   | .18  |
|     | 4.5. Verificação de Integridade                            | .19  |
|     | 4.6. Funções: Locais vs. Públicas                          | . 19 |
|     | 4.7. Constantes de Uma Biblioteca                          | .21  |
|     | 4.8. Struct de Dados Locais                                | 22   |
|     | 4.9. Uso de Tipos Definidos                                | 22   |
|     | 4.10. Uso de true e false para booleanos                   | 23   |
|     | 4.11. Números Mágicos                                      | .24  |
|     | 4.12. Utilização de BSP                                    | .25  |
|     | 4.13. Injeção de Dependência via Ponteiros de Função        | .26  |
| 5 - | - BOAS PRÁTICAS PARA MICROCONTROLADORES                    | 27   |
|     | 5.1. Filtragem de Sinais Analógicos                        | . 27 |

![](_page_2_Picture_1.jpeg)

## **1 – OBJETIVO**

Padronizar o desenvolvimento de firmware no Laboratório Assert, focando em estilos de codificação de simples interpretação, fácil manutenção e robustez de código. As diretrizes a seguir descrevem, com base no know-how coletivo dos projetistas de firmware do laboratório, as melhores formas de se escrever um código-fonte para sistemas embarcados.

Nas seções deste documento estão descritas regras de estilo de codificação, convenções de nomenclatura, regras de documentação, regras de arquitetura e boas práticas de desenvolvimento. As regras devem ser tomadas como mínimas obrigatórias: qualquer documentação ou cuidado adicional é bem-vindo.

![](_page_3_Picture_1.jpeg)

## **2 – REGRAS DE ESTILO**

#### **2.1. Idioma**

• Utilizar nomes de variáveis, funções e #defines em inglês.

## **Exemplo**

```
void Temperature_SetValuesBsp(u8 chann, u16 adValue)
{
    #if dTEMPERATURE_ENABLE_MINIMUN_ASSERTION == dTRUE
        if(chann < dNUMBER_OF_TEMPERATURE_POINTS)
    #endif
    {
        temperature.channels[chann].rawData = adValue;
    }
}
```

## **Por quê?**

• Padroniza a leitura do código com o que é praticado mundialmente.

#### **2.2. Encoding**

- Utilizar em todos os arquivos o encoding UTF-8, a menos que estritamente necessário.
- Para arquivos onde for necessário utilizar outro encoding (ex.: bibliotecas de display gráfico com escrita textual), configurar diretamente no projeto dentro da IDE e comentar no README do projeto.

#### **Por quê?**

• Ao trocar de computador ou subir um arquivo no Git, os caracteres podem ficar desconfigurados.

#### **2.3. Convenção de Nomenclatura — Dados**

• Utilizar lowerCamelCase para nomes de variáveis, constantes e ponteiros de dados.

#### **Exemplo**

```
u8 myVar1;
u8 *codeTester;
modbusType_t modbusStruct;
const char sendString[] = "Padronizando :)";
```

## **Por quê?**

• Restringe-se o uso de lowerCamelCase apenas para variáveis e estruturas de dados. Assim, identifica-se com facilidade o que é função e o que é variável dentro de um código.

#### **2.4. Convenção de Nomenclatura — Arquivos**

• Utilizar UpperCamelCase para nomes de arquivos source (.c) e header (.h).

![](_page_4_Picture_1.jpeg)

#### **Exemplo**

```
ModbusSlave.c
ModbusSlave.h
AssertTypes.h
ItDoesNotWork.c
```

## **Por quê?**

• Permite diferenciar nomes de arquivos de forma única, ocupando poucos caracteres (alternativa ao snake\_case).

#### **2.5. Convenção de Nomenclatura — Funções**

- Para funções, o nome deve ser composto pelo nome do arquivo .c e pelo nome da função em si.
- O nome da função em si também deve seguir o UpperCamelCase.
- Para separar o nome do arquivo do nome da função, utilizar snake\_case (um underline).

## **Exemplo**

```
/* File: GuitarPlayer.c */
void GuitarPlayer_Play(u8 data)
{
    // ...
}
u8 GuitarPlayer_RecordSong(u8 *song)
{
    // ...
}
```

#### **Por quê?**

- Permite identificar com facilidade a qual arquivo a função pertence.
- A escolha do snake\_case como separador facilita a diferenciação entre o nome do arquivo e o nome da função.

#### **2.6. Convenção de Nomenclatura — Typedefs**

• Utilizar lowerCamelCase para o nome do tipo, mas manter a identificação de tipo como \_t ao final.

```
typedef enum
{
    eState1,
    eState2,
    eNumberOfStates
} stateMachine_t;

typedef struct
{
    char metallica;
    char ironMaiden;
} badassBands_t;
```

![](_page_5_Picture_1.jpeg)




- Mantém o lowerCamelCase em alusão a dados, não funções.
- Colocar o \_t ao fim facilita a identificação visual de que se trata de um tipo.
- Além disso, o \_t recebe realce de cor diferenciada como tipo na maioria das IDEs e editores.

## **2.7. Convenção de Nomenclatura — #defines**

- Utilizar um d minúsculo no início dos #defines.
- Demais caracteres em maiúsculo (UPPER CASE).
- Para a separação de palavras dentro do #define, utilizar underline (\_).
- Para #defines públicos de um header, utilizar primeiramente o nome do arquivo e depois o #define em si — similar à regra de nomes de funções.
- Para #defines locais em um source, não há necessidade de utilizar o nome do arquivo no início do #define.

## **Exemplo**

```
/* File: ModbusSlave.h */
#define dMODBUS_SLAVE_NUMBER_OF_REGISTERS 10
#define dMODBUS_SLAVE_ENABLE_FAST_MODE true
/* File: ModbusSlave.c */
#define dNUMBER_OF_RETRY 5
#define d10_HZ 10
```

#### **Por quê?**

- O d facilita a identificação imediata do #define.
- As letras maiúsculas seguem uma tradição já consolidada para identificação de #defines.
- Utilizar o nome do arquivo ao início, em #defines públicos, facilita a identificação de qual arquivo pertence o #define quando utilizado em meio a um trecho de código.

#### **2.8. Convenção de Nomenclatura — enums**

- Utilizar um e minúsculo no início das enumerações.
- Demais caracteres em maiúsculo (UPPER CASE).
- Para a separação de palavras dentro da enumeração, utilizar underline (\_).

```
typedef enum
{
    eSTATE_1,
```

![](_page_6_Picture_1.jpeg)

```
eSTATE_2,
    eNUMBER_OF_STATES
} stateMachine_t;
enum
{
    eTHERMO_TYPE_J,
    eTHERMO_TYPE_K,
    eTHERMO_TYPE_S
} thermocoupleType_t;
```

• O e ao início remete à regra dos #defines (que utilizam d). Dessa forma, identificamos com facilidade os dados enumerados.

#### **2.9. Indentação: geral**

• Indentar sempre com 4 espaços, e nunca com TABs (configurar a IDE para não inserir TABs).

#### **Exemplo**

```
void Bergamota(void)
{
    int howManyDoYouWant;
    while(fruits.timer)
    {
        howManyDoYouWant++;
    }
}
```

#### **Por quê?**

• Indentar com espaços auxilia na visualização do código nas diferentes plataformas em que ele possa ser aberto (Notepad, VSCode, Git, IDEs, etc.).

#### **2.10. Indentação: funções, condições e loops**

- Abrir chaves na linha abaixo da função ou declaração.
- Para switch, utilizar o comando break no mesmo nível de indentação do comando case.

```
const u8 myData[] =
{
    0x01,
    0x02,
    // ...
};
if(myDate == 0x10)
{
    // ...
}
while(myData > 0)
```

![](_page_7_Picture_1.jpeg)

```
{
    // ...
}
void main(void)
{
    // ...
    switch(myData)
    {
        case 0x01:
        break;
        case 0x02:
        break;
        // ...
    }
}
```

• Facilita o entendimento do contexto da função ou dos dados.

## **2.11. Indentação: especificamente condicionais**

- Nunca fazer o "if de uma linha só" nem com teste e execução na mesma linha, nem com a execução logo abaixo sem chaves. Sempre utilizar chaves após um if ou else.
- Organizar sequências de testes complexos e longos em mais de uma linha, prezando pela interpretação das lógicas envolvidas.
- Separar todo e qualquer teste dentro de um operador com parênteses.

#### **Exemplo do que NÃO fazer**

```
// NAO FAZER CODIGO DO TIPO
if(var > 2) var++;
if(var < 3)
    dFUNCION_RUN();
if((varDeNomeBemGrande > outraVarComNomeMaiorAinda) && (varDeNomeBemGrande2 <
(outraVarComNomeMaiorAinda + 2)) || (varDeNomeBemGrande > outraVarComNomeMaiorAinda) &&
(varDeNomeBemGrande2 < (outraVarComNomeMaiorAinda + 4)) || (varDeNomeBemGrande >
outraVarComNomeMaiorAinda) && (varDeNomeBemGrande2 < (outraVarComNomeMaiorAinda + 4)) )
{
    dFUNCION_RUN();
}
if(var < 3 && var2 > 4)
{
    var++;
}
```

#### **Exemplo correto**

```
if(var > 2)
{
    var++;
}
```

![](_page_8_Picture_1.jpeg)

```
if(var < 3)
{
    dFUNCION_RUN();
}
if( ((varDeNomeBemGrande > outraVarComNomeMaiorAinda) && (varDeNomeBemGrande2 <
(outraVarComNomeMaiorAinda + 2))) ||
    ((varDeNomeBemGrande > outraVarComNomeMaiorAinda) && (varDeNomeBemGrande2 <
(outraVarComNomeMaiorAinda + 4))) ||
    ((varDeNomeBemGrande > outraVarComNomeMaiorAinda) && (varDeNomeBemGrande2 <
(outraVarComNomeMaiorAinda + 4))) )
{
    dFUNCION_RUN();
}
if((var < 3) && (var2 > 4))
{
    var++;
}
```

- O uso de condições de uma linha pode gerar bugs difíceis de depurar. Um exemplo clássico é um if de uma linha que execute uma função-#define multilinhas: apenas a primeira linha passará pelo if, e as demais sempre serão executadas.
- A organização de condições que ocupem mais do que a tela exibe normalmente auxilia na interpretação da lógica.
- A organização de cada par condicional por parênteses auxilia na interpretação da lógica.

#### **2.12. Indentação: diretivas de pré-compilação condicionais**

• Tratar diretivas de pré-compilação como literais if, mantendo a indentação.

## **Exemplo**

```
#if dENABLE_FEATURE
    #if dCODE_IS_GOOD
        executeFeature();
    #endif
#endif
```

#### **Por quê?**

• Dessa forma, a interpretação das condições de diretivas condicionais fica mais assertiva.

![](_page_9_Picture_1.jpeg)

# 3 - REGRAS DE DOCUMENTAÇÃO

As regras de documentação foram criadas focando nos principais objetivos:

- Bom entendimento do código por qualquer programador.
- Perpetuação das informações no laboratório.
- Proporcionar um momento de reflexão do programador: ao documentar, ele pode avaliar criticamente o código.

Além disso, foca-se em gerar a documentação no formato Doxygen. Assim, existe uma padronização de como e o que documentar, podendo gerar posteriormente documentações interativas e gráficas. Mesmo quando o HTML do Doxygen não é gerado, o formato obriga a seguir um padrão; e, sempre que necessário ou desejado, essa geração traz informações de forma facilitada a outros programadores.

Um ponto importante: não documentamos para nós. Documentamos para que o próximo programador possa entender o código sem que seja necessária uma explicação. Assim, a informação está completamente retida no projeto.

#### 3.1. Formato de comentários

- //: utilizar para comentários pontuais explicativos ao leitor do código.
- ///: utilizar para descrições de #defines, structs e enumerações em formato Doxygen.
- /\* \*/: utilizar para descrições de arquivos, funções e seções em formato Doxygen.
- Nunca utilizar caracteres especiais (acentuação, etc.) em comentários.
- Sempre que possível, comentar acima da linha de código e não ao lado, para facilitar a leitura.
- Comentários sempre em português. O código é elaborado em inglês por padronização, mas o entendimento mais complexo deve ser dado em português.

```
/*************************************
```

![](_page_10_Picture_1.jpeg)

```
static const u8 libData = 0;
```

- Separar os tipos de comentários auxilia na objetividade. Utiliza-se três barras para comentários que serão renderizados pelo Doxygen, e duas barras para comentários que auxiliam na explicação do código em si.
- O uso restrito de /\* \*/ para Doxygen faz com que ele não se confunda com comentários menores, facilitando sua identificação.

#### 3.2. Cabeçalhos de Arquivos

- Documentar o arquivo .c e apenas adicionar ao mesmo grupo Doxygen o arquivo .h.
   Isso evita duplicação de comentários na geração do Doxygen.
- Para sources de aplicação, não é necessário Changelog nem @copyright.
- Para bibliotecas, preencher corretamente o Changelog e o @copyright. O Changelog deve conter um resumo das alterações por versão; o @copyright deve conter o link do repositório da biblioteca no Git do Assert.

## Exemplo — arquivo .c

```
/**************************************
* @file ModbusSlave.c
* @addtogroup ModbusSlave
* @brief Modbus RTU para dispositivos escravos.
* @author Fulano de Tal
* @details
* \n <b>Ferramentas:</b>
  - Generic.
* \n <b>Dependencias:</b>
  - None
* \n <b>Observacoes:</b>
* - Devido ao tamanho atual das variaveis, sao permitidos apenas 255 registros
   cadastrados;
* - A biblioteca esta estruturada apenas para trabalhar em LITTLE ENDIAN;
* - Esta biblioteca foi feita para utilizacao em dispositivos com recursos
   limitados. Por isso, pode-se definir por meio do #define
   dMODUBS_RX_DATA_MAX_REGISTERS quantos registros podem ser escritos ou
lidos em um unico comando.
* Changelog
* @version <b>1.0.0 - 18/07/2019</b> \n Fulano de Tal \n Primeira versao.
* @version <b>1.1.0 - 23/11/2020</b> \n Fulano de Tal \n Adequacao doxygen
          e criacao da configuracao automatica de registros.
* @version <b>2.0.0 - 22/03/2023</b> \n Fulano de Tal \n Correcao da leitura
          de registros do tipo INPUT; remocao de dependencias externas.
*\ @ copyright \ https://git.assert.ifpb.edu.br/Assert/Modbus-RTU-Slave
      ***************************************
```

#### Exemplo — arquivo .h

![](_page_11_Picture_1.jpeg)

```
/****************/
/**

* @file ModbusSlave.h

* @addtogroup ModbusSlave

* @{
*********************************
```

- Todas as seções propostas foram pensadas para auxiliar na interpretação do arquivo.
- O versionamento, embora também ocorra no Git, é uma forma fácil de entender o que evoluiu na biblioteca quando ela é encontrada dentro de um código.

#### 3.3. Funções

- Todas as funções devem ter cabeçalhos, independentemente do seu tamanho ou propósito.
- Utilizar sempre as tags @brief (descrição sucinta do propósito da função), @param (descrição de cada um dos parâmetros da função — um para cada parâmetro) e @retval (retorno da função). Caso a função não tenha parâmetro ou não retorne nada, fica opcional utilizar a tag respectiva.
- Utilizar quando necessário e útil: @details (funcionamento interno da função ou explicação da interação dos parâmetros — usar sempre que a função for complexa) e @warning (alguma condição ou ponto de atenção).
- Demais tags do Doxygen, de livre uso quando necessário.

```
/**************************************
/** @brief Temporizacao principal da modubs.
* @param Nenhum.
   @retval Nenhum.
* @details Base de tempo para verificacao do tempo de parada. Este tempo deve
            ser de 3,5 vezes o tempo de transmissao de 11 bits. Utilizar em
           conjunto com o #define dMODBUS BREAK TICKS para que apos essa
           contagem o tempo final seja o tempo padrao de parada.
           Para taxas acima de 19200, utiliza-se o tempo calculado para 19200.
           Por exemplo:
               Taxa | T 11 bits | T 3,5x
               2400 | 4,583 ms | 16,042 ms
               4800 | 2,292 ms | 8,021 ms
9600 | 1,146 ms | 4,010 ms
19200 | 573 us | 2,005 ms
* @warning Esta funcao deve ser implementada em um timer de frequencia
          dMODBUS_TICK_FREQ Hz.
************************************
void Modbus Tick(void)
/**************************************
/** @brief Configura a string que sera enviada pelo protocolo nos campos de
     identificacao obrigatoria.
* @param field: campo segundo a enum modbus_obrigatoryFields_t;
```

![](_page_12_Picture_1.jpeg)

```
# @param strPtr: ponteiro do buffer contendo a string (ou apenas os bytes
```

- O intuito é ter um código particionado, em que cada função consiga ser suficiente e autoexplicativa.
- O objetivo é que o cabeçalho explique o funcionamento da função sem que o próximo programador precise abrir o código. Apenas com o Doxygen deve ser suficiente.

#### 3.4. Documentação de Configurações

 Documentar configurações (#defines) ao lado, explicitando sua unidade de medida ou faixa de valores aceitos.

```
/**************************************
* CONFIGURACOES
              ***************************************
/** @addtogroup modbus_appCfg Configuracoes da aplicacao.
* @brief Define as constantes do periferico slave e nuances de implementacao.
* @{
***********************************
/// Definicao do endianness do microcontrolador target.
/// @warning STM8 e big-endian, portanto deve ser dFALSE.
#define dMODBUS_USE_LITTLE_ENDIAN
/// OBS: o endereco zero (0) e reservado para o broadcast do master para todos
/// os slaves. Alem disso, os enderecos de 248 a 255 sao reservados pelo padrao
/// do protocolo. Portanto, dMODBUS_DEFAULT_SLAVE_ADDR pode ir apenas de 1 a 247.
#define dMODBUS DEFAULT SLAVE ADDR
                                    170
                                             // [byte]
/// Tempo ate o evento de break
#define dMODBUS_TIMEOUT_MS
                                      250
                                              // [ms]
/// Frequencia de chamada da rotina de temporizacao do modulo
#define dMODBUS_TICK_FREQ
                                     1000 // [Hz]
/// Baudrate da modbus
#define dMODBUS BAUD
                                      9600
                                             // [2400, 4800, 9600 ou 19200]
/// Definicao do CRC8 (versao simplificada para produtos de baixa memoria e
/// poder de processamento) ou CRC16 (padrao do protocolo modbus RTU)
#define dMODBUS CRC
                                              // [8 ou 16]
/// Numero de registros de dados maximo a serem recebidos.
/// OBS: aqui nao contam os dados do protocolo!
/// Caso esse numero seja ultrapassado, a mensagem recebida nao sera interpretada.
#define dMODUBS_RX_DATA_MAX_REGISTERS 16 // [Registros]
```

![](_page_13_Picture_1.jpeg)

• Facilita o entendimento rápido de como utilizar a configuração.

![](_page_14_Picture_1.jpeg)

## 4 - REGRAS DE ARQUITETURA

#### 4.1. Filosofia

A filosofia de arquitetura do código deve seguir os princípios:

- Criar um arquivo para cada conjunto de funcionalidade.
- Sempre que uma funcionalidade for replicável, criar uma biblioteca.
- Em bibliotecas, nunca alterar o arquivo .c. As configurações de aplicação e portabilidade devem sempre estar por completo no .h.
- Variável global deve ser utilizada como último recurso, em caso de falta de memória para fazer o código direito.
- Nunca fazer loops infinitos dentro do código, a menos que estritamente necessário.
- Utilizar o maior número possível de bibliotecas prontas. As bibliotecas obrigatórias são: AssertTypes e Version.

#### 4.2. Seções de Código

- Utilizar seções pré-definidas para escrever o código. O código não deve ser escrito fora dessas seções.
- Para .c: Includes, Defines locais, Constantes, Estruturas de dados locais, Protótipos locais, Funções públicas e Funções locais.
- Para .h: Includes necessários, Configurações, Defines públicos, Tipos de dados públicos e Protótipos públicos.

#### Exemplo — Source .c

```
/**************************************
* @file LibModel.c
* @addtogroup LIB MODEL
* @brief Padronizacao dos campos de comentario para agrupamento de codigo
       e padronizacao.
* @author Fulano de Tal
  @details
* \n <b>Ferramentas:</b>
* - Generic.
* \n <b>Dependencias:</b>
* - None.
* \n <b>Observacoes:</b>
* Changelog
* @version <b>1.0.0 - 03/08/2023</b> \n Fulano de Tal \n Primeira versao.
* @copyright https://git.assert.ifpb.edu.br/Assert/Flash-Eeprom
          ***************************************
/**************************************
* INCLUDES
```

![](_page_15_Picture_1.jpeg)

```
*************************************
#include "LibModel.h"
/**************************************
* DEFINES LOCAIS (fixos, apenas auxiliar para calculos)
/// Descricao do define
#define dLIB_MODEL_INIT
                    "ASSERT"
/**************************************
* CONSTANTES
          ***************************************
/// Descricao da constante
static const u8 libData = 0;
/**************************************
* ESTRUTURAS DE DADOS LOCAIS
     ·*************************************
static struct
  /// Descricao da variavel.
  u8 data;
} libModel;
/**************************************
* PROTOTIPOS LOCAIS
                ***************************************
static void LibModel_LocalFcn(void);
/**************************************
* FUNCOES PUBLICAS
. . . . . . . . . . . . . . . . . . . . 
/**************************************
/** @brief Resumo do objetivo da funcao.
* @param par1: descricao do parametro 1;
* @param par2: descricao do parametro 2.
* @retval descricao do retorno.
                             ***************************************
u8 LibModel_PublicFcn(u8 par1, u16 par2)
{
  return 0:
}
/**************************************
* FUNCOES LOCATS
***************************************
static void LibModel LocalFcn(void)
{
  // dummy
/** @} DOXYGEN GROUP TAG END OF FILE */
```

#### Exemplo — Header .h

```
/*************************************
```

![](_page_16_Picture_1.jpeg)

```
/***********************************
* INCLUDES NECESSARIOS
                ***************************************
#include "AssertTypes.h"
/**************************************
* CONFIGURACOES
***************************************
#define dENABLE_LIB_CALLS
                     dTRUE
/**************************************
* DEFINES PUBLICOS
            --
:***********************************
#define dLIB_MAX_TYPES
/**************************************
* TIPOS DE DADOS PUBLICOS
                 ---
**********************************
/// Tipos de sensores disponiveis segundo as configuracoes
typedef enum
  eMY_TIPO,
  eSU_TIPO
} libType_t;
/**************************************
* PROTOTIPOS PUBLICOS
               ***************************************
u8 LibModel_PublicFcn(u8 par1, u16 par2);
#endif /* _LIB_MODEL_H_ */
/** @} DOXYGEN GROUP TAG END OF FILE */
```

 A motivação vem das bibliotecas geradas pelos fabricantes de microcontroladores (Renesas, ST etc.), onde todos os códigos seguem exatamente este padrão. Identificam-se com facilidade as seções e sabe-se exatamente onde procurar cada parte do código.

#### 4.3. Includes

- No header, ter apenas os #includes necessários para a interpretação de tipos externos herdados para a interpretação exclusiva do conteúdo do header. Na maioria dos casos, é preciso no header apenas a biblioteca padrão de tipos AssertTypes.
- No source devem ser incluídos todos os headers necessários para definição de macros, tipos, protótipos etc.
- É importante que o primeiro arquivo .h incluído no source seja o self-header: o próprio arquivo header da biblioteca ou do arquivo de aplicação.
- Os demais includes devem aparecer em ordem alfabética.

#### Exemplo — Bsp.h

```
/*************************************
```

![](_page_17_Picture_1.jpeg)

```
* INCLUDES
************************************
```

#### Exemplo - Bsp.c

```
/*************************************
```

## 4.4. Defines locais vs. defines públicos

- Todos os #defines utilizados apenas dentro do código (arquivo de aplicação ou de bibliotecas) devem estar dentro do arquivo .c.
- Todos os #defines utilizados para configuração, lógicas de pré-compilação dentro do header, ou que precisarem ser acessados por outros arquivos, devem estar dentro do arquivo .h.
- É imprescindível que não sejam postos no .h os #defines que não sejam configurações ou que precisem ser acessados por outros arquivos.

#### Exemplo — arquivo .c

```
/*************************************
```

#### Exemplo — arquivo .h

```
/*************************************
```

![](_page_18_Picture_1.jpeg)

```
/// Habilita ou nao o tipo de sensor PT-100
#define dENABLE_PT100
```

- Fica claro quais informações são pertinentes a arquivos diversos e quais são exclusivas do source.
- Configurações ficam claras, e definições que não devem ser alteradas permanecem intocadas no .c.

#### 4.5. Verificação de Integridade

- Sempre que houver configurações que possam ser feitas de forma incorreta, fora de range ou com combinações proibidas, devem ser feitas diretivas de pré-compilação para gerar erros e bloquear a compilação caso seja feita uma configuração incorreta.
- Estas verificações devem ser feitas no arquivo .c.

## Exemplo

```
/************************************
```

#### Por quê?

 Geralmente é possível evitar um erro inesperado de execução, ou mesmo de compilação, com uma mensagem muito mais amigável e que auxilie na resolução desse erro.

#### 4.6. Funções: Locais vs. Públicas

Sempre que funções forem acessadas somente pelo source, devem ser definidas como static, evitando que sejam acessadas de fora do arquivo. Devem ter os protótipos declarados na área de "PROTÓTIPOS LOCAIS" e seu corpo de função na área de "FUNÇÕES LOCAIS".

![](_page_19_Picture_1.jpeg)

- Funções que devem ser acessadas por outros arquivos devem ser declaradas sem o atributo static, com seus protótipos apenas no arquivo header, na seção "PROTÓTIPOS PÚBLICOS", e o corpo da função na seção "FUNÇÕES PÚBLICAS" dentro do source.
- É imprescindível que não sejam configuradas como funções públicas aquelas que não têm finalidade de acesso fora do arquivo source em questão.

## Exemplo — Função pública

```
/* No header (.h): */
                   ***************************************
* PROTOTIPOS PUBLICOS
void Temperature_SetValuesBsp(u8 chann, u16 adValue);
/* No source (.c): */
                    ********************
* FUNCOES PUBLICAS
***************************************
/**************************************
/** @brief Envio dos valores de AD do hardware para a biblioteca.
* @param chann: canal a ser enviado o valor AD.

* @param adValue: valor lido pelo AD do microcontrolador, padronizado em
                  14 bits para a correta e precisa conversao de dados.
* @retval Nenhum.
void Temperature SetValuesBsp(u8 chann, u16 adValue)
   #if dTEMPERATURE ENABLE MINIMUN ASSERTION == dTRUE
      if(chann < dNUMBER_OF_TEMPERATURE_POINTS)</pre>
   #endif
       temperature.channels[chann].rawData = adValue;
   }
}
```

## Exemplo — Função local

```
/* No source (.c): */
/**********************************
```

![](_page_20_Picture_1.jpeg)

```
float temp = calibratedAdValue;
float result;

#if dENABLE_CUSTOM_THERMISTOR == dTRUE
    if(temperature.channels[chann].sensType == eCUSTOM_THERMISTOR)
```

• Evita-se o acesso a partes do código de forma errônea. Apenas as funções públicas têm escopo acessível.

#### 4.7. Constantes de Uma Biblioteca

 Constantes devem, sempre que possível (salvo exceções de memória com limitação ou velocidade de acesso em aplicações críticas), estar dentro do source com acesso restrito por instruções get\_xxx. Desta forma, a regra de negócio estará bem definida, facilitando o entendimento e o acesso.

#### Exemplo

```
/**************************************
* CONSTANTES
***************************************
/// Limite de temperatura para geracao do erro de temperatura baixa
/// (falha de sensor). Unidade: decimos de oC
const s16 temperatureLimitLow[] =
   #if dENABLE_TYPE_J == dTRUE
      -100,
   #endif
   #if dENABLE TYPE K == dTRUE
      -300
   #endif
};
/* · · · · */
/**************************************
* FUNCOES PUBLICAS
***************************************
/**************************************
/** @brief Acessa os limites superior e inferior de temperatura em decimos de
* grau celsius.
* @param sensType: tipo de sensor a ser acessado o limite de temperatura.
  @param limit: limite de temperatura a ser acessado (hi ou low).
* @retval Limite de temperatura selecionado, em s16.
**************************************
s16 Temperature_GetTemperatureLimits(sensor_t sensType, temperatureLimit_t limit)
   /* ··· */
}
```

#### Por quê?

 Utilizar #defines no header para este tipo de dado pode confundi-lo com uma configuração.

![](_page_21_Picture_1.jpeg)

 Assim, mantém-se a "integridade" de um source intocado, acessível apenas por funções.

#### 4.8. Struct de Dados Locais

 Todas as variáveis locais dentro de uma biblioteca ou arquivo de aplicação, de escopo global somente dentro do arquivo .c em questão, devem ser declaradas dentro de uma struct com nome igual ao do arquivo source.

#### Exemplo — Temperature.c

```
/**************************************
* ESTRUTURAS DE DADOS LOCAIS
            ***************************************
/// Variaveis internas da biblioteca
static struct
   struct
       /// Tipo do sensor do canal X
       sensor_t sensType;
       /// Dado no formato 14 bits (oversampling ou downsampling deve ser feito antes!)
       u16 rawData;
       /// Temperatura ja convertida e filtrada, dada em decimos de graus Celsius
       s16 temperature;
       /// Status deste canal de temperatura: OK ou erro (fora dos limites previstos)
       sensorStatus_t status;
       #if dTHERMOCOUPLE_OR_PTC_ENABLED == dTRUE
           /// Ponteiro da configuração de calibração.
           s16 *calibrationCfgPtr;
       #endif
   } channels[dNUMBER_OF_TEMPERATURE_POINTS];
   /// Flag alimentada pela aplicacao (bsp) para informar a lib que todos os
   /// canais ad (RAW) foram alimentados
   bool adReady;
   #if dTHERMOCOUPLE OR PTC ENABLED == dTRUE
       /// Flag que indica que a calibracao esta ativa
       bool isCalibrating;
       /// Ponteiro para o estado de calibracao, definido da aplicacao
       u8 *calState:
       /// Contador de ticks para a estabilizacao do sinal AD no momento da calibracao
       u8 calTickCount;
   #endif
} temperature;
```

#### Por quê?

• Dentro das funções, pode-se distinguir o que é variável de escopo local da função e de escopo amplo do arquivo .c.

#### 4.9. Uso de Tipos Definidos

![](_page_22_Picture_1.jpeg)

 Todas as variáveis que forem referentes a tipos de dados em relação aos números específicos — sejam tipagens apenas para diferenciação de estados, definição de possibilidades de parâmetros ou de definição explícita de números — devem ser utilizadas com typedefs enum.

## Exemplo — definição de parâmetros aceitos por uma função

```
/// Possiveis limites de temperatura a serem acessados
typedef enum
   eTEMPERATURE LIMIT LOW,
   eTEMPERATURE_LIMIT_HIGH
} temperatureLimit_t;
/* ... */
/**************************************
/** @brief Acessa os limites superior e inferior de temperatura em decimos de
         grau celsius.
* @param sensType: tipo de sensor a ser acessado o limite de temperatura.
* @param limit: limite de temperatura a ser acessado (hi ou low).
   @retval Limite de temperatura selecionado, em s16.
                        ***************************************
s16 Temperature_GetTemperatureLimits(sensor_t sensType, temperatureLimit_t limit)
{
   /* ··· */
}
```

#### Exemplo — explicitação numérica

```
/// Enderecos dos registros da modbus\nenum
{
```

#### Por quê?

 Fica explícita a função da variável declarada com este tipo, além de facilitar a leitura do código.

#### 4.10. Uso de true e false para booleanos

- Sempre que uma variável ou condição for estritamente booleana (resultado true ou false), utilizar o tipo bool.
- Para comparações utilizando o tipo bool, ser explícito, comparando com o resultado desejado.
- Nunca utilizar uma variável de qualquer outro tipo ou tamanho para representar um estado que foi projetado para ser apenas verdadeiro ou falso.

![](_page_23_Picture_1.jpeg)

## **Exemplo**

```
// Retorna um boleano pois existem apenas 2 possibilidades de resultado
bool CodeWriter_CheckAlive(bool stillAlive, u32 data)
{
    bool imAlive = false;
    // Explicitamente verifica se o valor e maior do que zero.
    // Nao faz um if(data) sem objetivo.
    if(data > 0)
    {
        // Compara explicitamente com a condicao "verdade"
        if(stillAlive == true)
        {
            imAlive = true;
        }
    }
    return imAlive;
}
```

#### **Por quê?**

- Comparações explícitas auxiliam no entendimento do código.
- Utilizar booleanos restringe as opções de uma variável, facilitando o entendimento e garantindo a correta utilização da variável.

### **4.11. Números Mágicos**

- Nunca utilizar números soltos no código. Sempre deve haver um #define ou uma constante atrelada ao número.
- Ao utilizar máscaras para lógicas bit a bit, explicar em comentário na linha superior do código seu intuito.

## **Exemplo do que NÃO fazer**

```
for(i = 0; i < 16; i++)
{
    if(shrAllOutputs & 0x8000)
    {
        shrHandler.writeDataPin(true);
    }
    else
    {
        shrHandler.writeDataPin(false);
    }
    Shr_ClockPinPulse();
    shrAllOutputs = (shrAllOutputs << 1);
}
```

## **Exemplo correto**

```
for(i = 0; i < dNUMBER_OF_SHR_BITS; i++)
{
    // Acessa o bit de numero 15 (primeiro bit da esquerda em 16 bits)
    if(shrAllOutputs & 0x8000)
    {
        shrHandler.writeDataPin(true);
    }
    else
```

![](_page_24_Picture_1.jpeg)

```
{
    shrHandler.writeDataPin(false);
}
Shr_ClockPinPulse();
shrAllOutputs = (shrAllOutputs << 1);
}</pre>
```

 Dificilmente alguém entende, olhando apenas um trecho de código, a intenção do desenvolvedor original. Assim, mantém-se explícito o que se quer fazer com cada número do código.

## 4.12. Utilização de BSP

- Realizar toda configuração do hardware de baixo nível nos arquivos de BSP.
- Para a interface com o main.c, disponibilizar a função Bsp\_Init(). Essa deve ser a única interface da aplicação com a inicialização do hardware.
- Se utilizar bibliotecas externas, chamá-las de dentro dos arquivos de BSP.
- **Modularizar o BSP por periférico.** Não concentrar toda a configuração de baixo nível (ADC, SPI, UART, USB, etc.) num único Bsp.c: isso faz o arquivo crescer demais e mistura assuntos sem relação entre si, dificultando manutenção.
- Para cada periférico ou domínio de hardware, criar um par `Bsp<Periferico>.c` / `Bsp<Periferico>.h` (ex.: `BspUart.c`, `BspAdc.c`, `BspSpi.c`), seguindo as mesmas regras de arquitetura, nomenclatura e documentação de qualquer outro arquivo do projeto.
- O `Bsp.c` deixa de conter a configuração de hardware em si e passa a ser um **orquestrador**: dentro de `Bsp_Init()`, ele apenas chama o `Init` de cada `Bsp<Periferico>` específico.
- O `main.c` continua chamando apenas `Bsp_Init()`. Ele nunca deve chamar diretamente um `Bsp<Periferico>_Init()`.

#### Exemplo — main.c

```
void main(void)
{
    /* ... */
    Bsp_Init();

    while(1)
    {
        /* ... */
    }
}
```

#### Exemplo — Bsp.c (orquestrador)

```
/**************************************
/** @brief Inicializacao dos perifericos do hardware, delegando para os
           Bsp especificos de cada periferico.
* @param Nenhum.
* @retval Nenhum.
************************************/
void Bsp_Init(void)
{
    BspUart_Init();
    BspAdc_Init();
    BspSpi_Init();
}
```

#### Exemplo — BspUart.c (BSP específico)

```
/**************************************
/** @brief Inicializacao de baixo nivel da Uart e injecao dos ponteiros de
           funcao na lib Uart (ver regra 4.13).
* @param Nenhum.
* @retval Nenhum.
************************************/
void BspUart_Init(void)
{
    /* Configuracao de baixo nivel do periferico (clock, pinos, registradores) */

    Uart_Init(BspUart_Send, BspUart_Receive);
}
```

#### Por quê?

- A aplicação não se detém à interface com o hardware, nem mesmo para realizar os inits das bibliotecas do micro.
- Fica fácil encontrar onde estão as interfaces com o hardware, padronizando sempre nos arquivos de BSP.
- Modularizar por periférico evita um único arquivo gigante e sem coesão: cada `Bsp<Periferico>` trata de um assunto só, ficando mais fácil de localizar, revisar e manter.
- Isolar cada periférico em seu próprio arquivo facilita reaproveitar o `Bsp<Periferico>` em outro projeto que use o mesmo hardware, mesmo que o restante do BSP mude.

![](_page_25_Picture_1.jpeg)

![](_page_26_Picture_1.jpeg)

## **4.13. Injeção de Dependência via Ponteiros de Função**

- Bibliotecas que precisam acessar hardware (comunicação, periféricos, etc.) nunca devem incluir o `Bsp.h` diretamente.
- Em vez disso, a lib deve receber, através de sua função de inicialização (`Nome_Init()`), ponteiros para as funções de hardware de que necessita. Essas funções são implementadas no `Bsp.c` e repassadas para a lib no momento da inicialização.
- Os ponteiros recebidos devem ser armazenados numa sub-struct (ex.: `functions`) dentro da struct de dados locais da lib (ver regra 4.8).
- A função de inicialização deve validar se algum dos ponteiros recebidos é nulo, retornando um valor de erro apropriado (ex.: `eXXX_RETURN_INVALID_ARGUMENT`) nesse caso.
- Quem chama `Nome_Init()` (tipicamente o `Bsp.c` ou o arquivo de aplicação) é responsável por passar as funções de hardware compatíveis com a assinatura esperada.

#### **Exemplo — Uart.h**

```
typedef enum uartReturn
{
    eUART_RETURN_OK,
    eUART_RETURN_INVALID_ARGUMENT,
    eUART_RETURN_ERROR,
    eUART_RETURN_IDLE,

    eUART_RETURN_END_ENUM
} uartReturn_t;

uartReturn_t Uart_Init(void (*uartSendFunc)(u8 *txBuffer, u16 txBufferSize),
                        void (*uartReceiveFunc)(u8 *rxBuffer, u16 rxBufferSize));
```

#### **Exemplo — Uart.c**

```
static struct uart
{
    /// @brief Estrutura com os ponteiros de funcoes externas (HAL) da lib
    struct functions
    {
        void (*uartSend)(u8 *txBuffer, u16 txBufferSize);
        void (*uartReceive)(u8 *rxBuffer, u16 rxBufferSize);
    } functions;

} uart;

uartReturn_t Uart_Init(void (*uartSendFunc)(u8 *txBuffer, u16 txBufferSize),
                       void (*uartReceiveFunc)(u8 *rxBuffer, u16 rxBufferSize))
{
    if (uartSendFunc == dNULL || uartReceiveFunc == dNULL)
    {
        return eUART_RETURN_INVALID_ARGUMENT;
    }

    uart.functions.uartSend    = uartSendFunc;
    uart.functions.uartReceive = uartReceiveFunc;

    return eUART_RETURN_OK;
}
```

#### **Por quê?**

- Desacopla completamente a lib da implementação de hardware: a mesma lib pode ser reaproveitada em outro microcontrolador ou projeto bastando passar novos ponteiros de função compatíveis com a assinatura esperada.
- Reduz o acoplamento de includes (ver regra 4.3): o header da lib continua precisando apenas de `AssertTypes.h`, sem depender de `Bsp.h`.
- Facilita testes: é possível injetar funções mock/stub sem necessidade de hardware real.

![](_page_27_Picture_1.jpeg)

# **5 – BOAS PRÁTICAS PARA MICROCONTROLADORES**

#### **5.1. Filtragem de Sinais Analógicos**

- Realizar a filtragem de todo e qualquer sinal analógico medido pelo microcontrolador. O filtro deve ser projetado de acordo com a aplicação.
- Sugere-se o uso de média móvel para aplicações comuns. Uma biblioteca de média móvel simples (Simple-Moving-Average) pode ser disponibilizada no repositório do Assert para uso direto.

## **Exemplo — main.c**

```
simpleMovingAvg_t myMove;
s16 myVar = 0;
bool isFull = false;
for(;;)
{
    SimpleMovingAvg_NewSample(&myMove, 1000);
    myVar = SimpleMovingAvg_GetChannValue(&myMove);
    isFull = SimpleMovingAvg_IsBufferFull(&myMove);
}
```

#### **Por quê?**

• Ler sinais diretamente convertidos pelo AD sem nenhuma filtragem é extremamente suscetível a ruídos. É necessário realizar pelo menos uma média no sinal.