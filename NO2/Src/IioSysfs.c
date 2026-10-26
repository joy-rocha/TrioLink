/**************************************
* @file IioSysfs.c
* @addtogroup IioSysfs
* @brief Acesso generico a atributos de dispositivos IIO expostos pelo kernel Linux via sysfs.
* @author Caio Oliveira
* @details
* \n <b>Ferramentas:</b>
* - Raspberry Pi 5, Linux (subsistema IIO).
* \n <b>Dependencias:</b>
* - AssertTypes.
* \n <b>Observacoes:</b>
* - Esta biblioteca nao conhece nenhum sensor especifico; apenas localiza
*   dispositivos IIO pelo nome e le atributos numericos (raw, scale, offset).
* - Qualquer biblioteca especifica de sensor (ex.: Mpu6500Iio) deve ser
*   construida sobre esta camada.
* - Reutilizavel por qualquer no do projeto que utilize sensor via IIO
*   (ex.: BMP280 do No 1), bastando trocar o nome de dispositivo procurado.
* Changelog
* @version <b>1.1.0 - 26/09/2026</b> \n Caio Oliveira \n Segunda versao.
* @copyright 
**************************************/

/**************************************
* INCLUDES
**************************************/
#include "IioSysfs.h"
#include <dirent.h>
#include <stdio.h>
#include <string.h>

/**************************************
* DEFINES LOCAIS
**************************************/
/// Prefixo usado pelo kernel para nomear os diretorios de device IIO
#define dIIO_SYSFS_DEVICE_PREFIX       "iio:device" // [string]
/// Quantidade de caracteres do prefixo acima
#define dIIO_SYSFS_DEVICE_PREFIX_LEN   10            // [bytes]
/// Tamanho maximo esperado para o conteudo do arquivo "name" de um device
#define dIIO_SYSFS_DEVICE_NAME_MAX     64            // [bytes]

/**************************************
* CONSTANTES
**************************************/
/* Nao ha constantes internas nesta biblioteca */

/**************************************
* ESTRUTURAS DE DADOS LOCAIS
**************************************/
/* Esta biblioteca e sem estado: nao possui variaveis estaticas locais */

/**************************************
* PROTOTIPOS LOCAIS
**************************************/
/* Nao ha funcoes locais nesta biblioteca */

/**************************************
* FUNCOES PUBLICAS
**************************************/
/**************************************
/** @brief Localiza um dispositivo IIO cujo atributo "name" seja igual a
           nameMatch.
* @param nameMatch: nome exato do dispositivo a ser procurado (ex.: "mpu6500").
* @param devPathOut: buffer onde sera escrito o caminho completo do diretorio
                     do dispositivo encontrado, caso a busca seja bem sucedida.
* @param outLen: tamanho em bytes do buffer devPathOut.
* @retval eIIO_SYSFS_RETURN_OK se encontrado.
          eIIO_SYSFS_RETURN_NOT_FOUND se nenhum dispositivo casar com nameMatch.
          eIIO_SYSFS_RETURN_INVALID_ARGUMENT se algum parametro for invalido.
* @details Percorre dIIO_SYSFS_BASE_PATH comparando o conteudo do arquivo
           "name" de cada subdiretorio "iio:deviceN" com nameMatch. O indice N
           nao e confiavel entre boots, por isso a busca e sempre por nome.
**************************************/
iioSysfsReturn_t IioSysfs_FindDevice(const char *nameMatch, char *devPathOut, u16 outLen)
{
    iioSysfsReturn_t result = eIIO_SYSFS_RETURN_NOT_FOUND;

    if((nameMatch == dNULL) || (devPathOut == dNULL) || (outLen == 0))
    {
        result = eIIO_SYSFS_RETURN_INVALID_ARGUMENT;
    }
    else
    {
        DIR *dirHandle = opendir(dIIO_SYSFS_BASE_PATH);

        if(dirHandle != dNULL)
        {
            struct dirent *entry = readdir(dirHandle);

            while((entry != dNULL) && (result != eIIO_SYSFS_RETURN_OK))
            {
                // So avalia entradas que comecem com o prefixo "iio:device"
                if(strncmp(entry->d_name, dIIO_SYSFS_DEVICE_PREFIX, dIIO_SYSFS_DEVICE_PREFIX_LEN) == 0)
                {
                    char nameFilePath[dIIO_SYSFS_MAX_PATH];

                    snprintf(nameFilePath, sizeof(nameFilePath), "%s/%s/name", dIIO_SYSFS_BASE_PATH, entry->d_name);

                    FILE *nameFile = fopen(nameFilePath, "r");

                    if(nameFile != dNULL)
                    {
                        char deviceName[dIIO_SYSFS_DEVICE_NAME_MAX] = {0};

                        if(fgets(deviceName, sizeof(deviceName), nameFile) != dNULL)
                        {
                            // Remove a quebra de linha (\n) do final, se existir
                            deviceName[strcspn(deviceName, "\n")] = 0;

                            if(strcmp(deviceName, nameMatch) == 0)
                            {
                                snprintf(devPathOut, outLen, "%s/%s", dIIO_SYSFS_BASE_PATH, entry->d_name);
                                result = eIIO_SYSFS_RETURN_OK;
                            }
                        }
                        fclose(nameFile);
                    }
                }
                entry = readdir(dirHandle);
            }
            closedir(dirHandle);
        }
    }
    return result;
}

/**************************************
/** @brief Le um atributo numerico (ponto flutuante) de um dispositivo IIO.
* @param devPath: caminho do diretorio do dispositivo, obtido via
                  IioSysfs_FindDevice().
* @param attr: nome do arquivo de atributo a ser lido (ex.: "in_accel_scale").
* @param out: ponteiro onde o valor lido sera escrito.
* @retval eIIO_SYSFS_RETURN_OK se a leitura for bem sucedida.
          eIIO_SYSFS_RETURN_READ_ERROR se o arquivo nao existir ou o
          conteudo nao puder ser interpretado como numero.
          eIIO_SYSFS_RETURN_INVALID_ARGUMENT se algum parametro for invalido.
**************************************/
iioSysfsReturn_t IioSysfs_ReadAttrDouble(const char *devPath, const char *attr, double *out)
{
    iioSysfsReturn_t result = eIIO_SYSFS_RETURN_READ_ERROR;

    if((devPath == dNULL) || (attr == dNULL) || (out == dNULL))
    {
        result = eIIO_SYSFS_RETURN_INVALID_ARGUMENT;
    }
    else
    {
        char attrPath[dIIO_SYSFS_MAX_PATH];

        snprintf(attrPath, sizeof(attrPath), "%s/%s", devPath, attr);

        FILE *attrFile = fopen(attrPath, "r");

        if(attrFile != dNULL)
        {
            if(fscanf(attrFile, "%lf", out) == 1)
            {
                result = eIIO_SYSFS_RETURN_OK;
            }
            fclose(attrFile);
        }
    }
    return result;
}

/**************************************
/** @brief Le um atributo numerico (inteiro) de um dispositivo IIO.
* @param devPath: caminho do diretorio do dispositivo, obtido via
                  IioSysfs_FindDevice().
* @param attr: nome do arquivo de atributo a ser lido (ex.: "in_accel_x_raw").
* @param out: ponteiro onde o valor lido sera escrito.
* @retval eIIO_SYSFS_RETURN_OK se a leitura for bem sucedida.
          eIIO_SYSFS_RETURN_READ_ERROR se o arquivo nao existir ou o
          conteudo nao puder ser interpretado como numero.
          eIIO_SYSFS_RETURN_INVALID_ARGUMENT se algum parametro for invalido.
**************************************/
iioSysfsReturn_t IioSysfs_ReadAttrLong(const char *devPath, const char *attr, s32 *out)
{
    iioSysfsReturn_t result = eIIO_SYSFS_RETURN_READ_ERROR;

    if((devPath == dNULL) || (attr == dNULL) || (out == dNULL))
    {
        result = eIIO_SYSFS_RETURN_INVALID_ARGUMENT;
    }
    else
    {
        char attrPath[dIIO_SYSFS_MAX_PATH];

        snprintf(attrPath, sizeof(attrPath), "%s/%s", devPath, attr);

        FILE *attrFile = fopen(attrPath, "r");

        if(attrFile != dNULL)
        {
            if(fscanf(attrFile, "%d", out) == 1)
            {
                result = eIIO_SYSFS_RETURN_OK;
            }
            fclose(attrFile);
        }
    }
    return result;
}

/**************************************
* FUNCOES LOCAIS
**************************************/
/* Nao ha funcoes locais nesta biblioteca */
/** @} DOXYGEN GROUP TAG END OF FILE */
