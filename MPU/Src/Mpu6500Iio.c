/**************************************
* @file Mpu6500Iio.c
* @addtogroup Mpu6500Iio
* @brief Leitura do sensor MPU6500 (acelerometro e giroscopio) via subsistema
         IIO do kernel Linux, no Raspberry Pi 5, com calibracao de bias em
         software e leitura com media de multiplas amostras.
* @author Caio Oliveira
* @details
* \n <b>Ferramentas:</b>
* - Raspberry Pi 5, driver de kernel inv_mpu6050 (subsistema IIO).
* \n <b>Dependencias:</b>
* - AssertTypes.
* - IioSysfs.
* \n <b>Observacoes:</b>
* - A calibracao de bias (desvio de fabricacao) e feita EM SOFTWARE, dentro
*   desta biblioteca, e nao depende de nenhum arquivo de calibracao exposto
*   pelo driver no sysfs. Isso foi uma decisao deliberada: o dispositivo real
*   deste projeto expoe "in_accel_x_calibbias" (convencao IIO), nao
*   "in_accel_x_offset", e a forma como diferentes drivers aplicam esse
*   atributo internamente nao e padronizada o suficiente para confiar nele.
*   Calibrar em software funciona da mesma forma independente do driver.
* - Esta biblioteca assume um unico sensor fisico por no (singleton), e
*   assume calibracao de FABRICA do chip (nao recalibra a cada amostra).
* Changelog
* @version <b>1.0.0 - 26/09/2026</b> \n Caio Oliveira \n Primeira versao.
* @version <b>1.1.0 - 17/10/2026</b> \n Caio Oliveira \n Calibracao em
*          software (Calibrate/Save/Load) e leitura com media
*          (ReadAveragedSample). Removida a leitura de offset via sysfs,
*          que nunca encontrava o arquivo no hardware real do projeto.
* @copyright https://git.assert.ifpb.edu.br/Assert/<repositorio-do-projeto>
**************************************/

/**************************************
* INCLUDES
**************************************/
#include "Mpu6500Iio.h"
#include "IioSysfs.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

/**************************************
* DEFINES LOCAIS
**************************************/
/// Nomes dos arquivos de atributo lidos no sysfs do dispositivo IIO
#define dMPU6500_IIO_ATTR_ACCEL_SCALE       "in_accel_scale"     // [string]
#define dMPU6500_IIO_ATTR_GYRO_SCALE        "in_anglvel_scale"   // [string]

/// Prefixos de canal usados para montar o nome do atributo "raw" de cada eixo
#define dMPU6500_IIO_CHANNEL_ACCEL          "accel"              // [string]
#define dMPU6500_IIO_CHANNEL_GYRO           "anglvel"            // [string]

/// Tamanho maximo do nome de atributo montado dinamicamente (ex.: "in_accel_x_raw")
#define dMPU6500_IIO_ATTR_NAME_MAX          32                   // [bytes]

/// Aceleracao da gravidade padrao, usada para saber quanto "1g" vale em raw
/// no eixo que a calibracao detectar como estando apontado para cima
#define dMPU6500_IIO_STANDARD_GRAVITY_MPS2  9.80665              // [m/s^2]

/// Intervalo entre amostras durante a calibracao
#define dMPU6500_IIO_CALIB_SAMPLE_INTERVAL_MS  5                 // [ms]

/// Formato de uma linha do arquivo de calibracao salvo em disco: os 6
/// offsets (3 accel, 3 gyro), em unidades de raw (contagem do ADC)
#define dMPU6500_IIO_CALIB_FILE_FORMAT      "%ld %ld %ld %ld %ld %ld\n"

/**************************************
* CONSTANTES
**************************************/
/* Nao ha constantes internas nesta biblioteca */

/**************************************
* ESTRUTURAS DE DADOS LOCAIS
**************************************/
/// Variaveis internas da biblioteca (estado do sensor, descoberto em Init,
/// e offsets de calibracao, descobertos em Calibrate ou LoadCalibration)
static struct
{
    /// Caminho do diretorio do dispositivo IIO, descoberto em Mpu6500Iio_Init()
    char devPath[dIIO_SYSFS_MAX_PATH];
    /// Fator de conversao raw -> m/s^2 do acelerometro
    double accelScale;
    /// Fator de conversao raw -> rad/s do giroscopio
    double gyroScale;
    /// Offsets de calibracao (em unidades de raw), somados ao valor lido
    /// antes de multiplicar pela escala: fisico = (raw + offset) * scale
    s32 accelOffsetX;
    s32 accelOffsetY;
    s32 accelOffsetZ;
    s32 gyroOffsetX;
    s32 gyroOffsetY;
    s32 gyroOffsetZ;
    /// Indica se os offsets acima vieram de uma calibracao valida
    /// (Calibrate() ou LoadCalibration()). Enquanto false, sao todos zero.
    bool isCalibrated;
    /// Indica se Mpu6500Iio_Init() ja foi chamada com sucesso
    bool isInitialized;
} mpu6500Iio;

/**************************************
* PROTOTIPOS LOCAIS
**************************************/
static iioSysfsReturn_t Mpu6500Iio_LocateDevice(void);
static mpu6500IioReturn_t Mpu6500Iio_ReadRawAxis(const char *channelType, char axis, s32 *rawOut);
static mpu6500IioReturn_t Mpu6500Iio_ReadCalibratedAxis(const char *channelType, char axis, double scale, s32 offset, float *out);
static s64 Mpu6500Iio_GetTimestampMs(void);
static void Mpu6500Iio_SleepMs(u16 milliseconds);

/**************************************
* FUNCOES PUBLICAS
**************************************/
/**************************************
/** @brief Localiza o sensor MPU6500 no subsistema IIO e le seus fatores de
           escala. Nao realiza calibracao: isso e feito separadamente por
           Mpu6500Iio_Calibrate() ou Mpu6500Iio_LoadCalibration().
* @param Nenhum.
* @retval eMPU6500_IIO_RETURN_OK se o sensor foi encontrado e configurado.
          eMPU6500_IIO_RETURN_NOT_FOUND se nenhum dispositivo IIO casar com
          os nomes configurados (ver dMPU6500_IIO_PRIMARY_NAME e dMPU6500_IIO_FALLBACK_NAME).
          eMPU6500_IIO_RETURN_READ_ERROR se o dispositivo foi encontrado mas a leitura da escala falhou.
* @details Deve ser chamada uma unica vez antes de Mpu6500Iio_ReadSample(),
           Mpu6500Iio_ReadAveragedSample() ou Mpu6500Iio_Calibrate(). Os
           offsets de calibracao comecam zerados (sem correcao) ate que
           Calibrate() ou LoadCalibration() sejam chamadas.
**************************************/
mpu6500IioReturn_t Mpu6500Iio_Init(void)
{
    mpu6500IioReturn_t result = eMPU6500_IIO_RETURN_NOT_FOUND;

    memset(&mpu6500Iio, 0, sizeof(mpu6500Iio));

    if(Mpu6500Iio_LocateDevice() == eIIO_SYSFS_RETURN_OK)
    {
        if(IioSysfs_ReadAttrDouble(mpu6500Iio.devPath, dMPU6500_IIO_ATTR_ACCEL_SCALE, &mpu6500Iio.accelScale) != eIIO_SYSFS_RETURN_OK)
        {
            result = eMPU6500_IIO_RETURN_READ_ERROR;
        }
        else if(IioSysfs_ReadAttrDouble(mpu6500Iio.devPath, dMPU6500_IIO_ATTR_GYRO_SCALE, &mpu6500Iio.gyroScale) != eIIO_SYSFS_RETURN_OK)
        {
            result = eMPU6500_IIO_RETURN_READ_ERROR;
        }
        else
        {
            mpu6500Iio.isInitialized = true;
            result = eMPU6500_IIO_RETURN_OK;
        }
    }
    return result;
}

/**************************************
/** @brief Le uma unica amostra (acelerometro e giroscopio, 3 eixos cada),
           ja calibrada (se Calibrate()/LoadCalibration() tiver sido
           chamada) e convertida para unidades fisicas.
* @param sampleOut: ponteiro para a estrutura onde a amostra sera escrita.
* @retval eMPU6500_IIO_RETURN_OK se a leitura for bem sucedida.
          eMPU6500_IIO_RETURN_INVALID_ARGUMENT se sampleOut for dNULL.
          eMPU6500_IIO_RETURN_NOT_INITIALIZED se Init() ainda nao foi chamada.
          eMPU6500_IIO_RETURN_READ_ERROR se qualquer um dos seis eixos
          falhar na leitura. Neste caso, sampleOut nao deve ser usado.
* @details Uma unica leitura carrega todo o ruido eletrico instantaneo do sensor. 
           Para publicar um valor mais estavel, prefira Mpu6500Iio_ReadAveragedSample().
**************************************/
mpu6500IioReturn_t Mpu6500Iio_ReadSample(mpu6500Sample_t *sampleOut)
{
    mpu6500IioReturn_t result = eMPU6500_IIO_RETURN_INVALID_ARGUMENT;

    if(sampleOut == dNULL)
    {
        result = eMPU6500_IIO_RETURN_INVALID_ARGUMENT;
    }
    else if(mpu6500Iio.isInitialized == false)
    {
        result = eMPU6500_IIO_RETURN_NOT_INITIALIZED;
    }
    else
    {
        sampleOut->timestampMs = Mpu6500Iio_GetTimestampMs();

        result = Mpu6500Iio_ReadCalibratedAxis(dMPU6500_IIO_CHANNEL_ACCEL, 'x', mpu6500Iio.accelScale, mpu6500Iio.accelOffsetX, &sampleOut->accelX);

        if(result == eMPU6500_IIO_RETURN_OK)
        {
            result = Mpu6500Iio_ReadCalibratedAxis(dMPU6500_IIO_CHANNEL_ACCEL, 'y', mpu6500Iio.accelScale, mpu6500Iio.accelOffsetY, &sampleOut->accelY);
        }
        if(result == eMPU6500_IIO_RETURN_OK)
        {
            result = Mpu6500Iio_ReadCalibratedAxis(dMPU6500_IIO_CHANNEL_ACCEL, 'z', mpu6500Iio.accelScale, mpu6500Iio.accelOffsetZ, &sampleOut->accelZ);
        }
        if(result == eMPU6500_IIO_RETURN_OK)
        {
            result = Mpu6500Iio_ReadCalibratedAxis(dMPU6500_IIO_CHANNEL_GYRO, 'x', mpu6500Iio.gyroScale, mpu6500Iio.gyroOffsetX, &sampleOut->gyroX);
        }
        if(result == eMPU6500_IIO_RETURN_OK)
        {
            result = Mpu6500Iio_ReadCalibratedAxis(dMPU6500_IIO_CHANNEL_GYRO, 'y', mpu6500Iio.gyroScale, mpu6500Iio.gyroOffsetY, &sampleOut->gyroY);
        }
        if(result == eMPU6500_IIO_RETURN_OK)
        {
            result = Mpu6500Iio_ReadCalibratedAxis(dMPU6500_IIO_CHANNEL_GYRO, 'z', mpu6500Iio.gyroScale, mpu6500Iio.gyroOffsetZ, &sampleOut->gyroZ);
        }
    }
    return result;
}

/**************************************
/** @brief Le numSamples amostras, espacadas por sampleIntervalMs, e devolve
           a MEDIA de cada eixo. Reduz o ruido de leitura em troca de um
           pequeno atraso e de perder variacoes muito rapidas dentro da
           janela de amostragem.
* @param numSamples: quantidade de leituras a serem feitas e promediadas.
* @param sampleIntervalMs: intervalo entre cada leitura, em milissegundos
* @param sampleOut: ponteiro para a estrutura onde a media sera escrita.
* @retval eMPU6500_IIO_RETURN_OK se todas as leituras tiveram sucesso.
          eMPU6500_IIO_RETURN_INVALID_ARGUMENT se sampleOut for dNULL ou numSamples for zero.
          eMPU6500_IIO_RETURN_NOT_INITIALIZED se Init() ainda nao foi chamada.
          eMPU6500_IIO_RETURN_READ_ERROR se qualquer leitura individual falhar.
          Neste caso, sampleOut nao deve ser usado.
* @details O timestamp da amostra resultante e o do INICIO da janela (a
           primeira leitura), nao uma media de timestamps: representa "a
           partir de quando" aquele dado passou a ser coletado.
**************************************/
mpu6500IioReturn_t Mpu6500Iio_ReadAveragedSample(u16 numSamples, u16 sampleIntervalMs, mpu6500Sample_t *sampleOut)
{
    mpu6500IioReturn_t result = eMPU6500_IIO_RETURN_INVALID_ARGUMENT;

    if((sampleOut == dNULL) || (numSamples == 0))
    {
        result = eMPU6500_IIO_RETURN_INVALID_ARGUMENT;
    }
    else if(mpu6500Iio.isInitialized == false)
    {
        result = eMPU6500_IIO_RETURN_NOT_INITIALIZED;
    }
    else
    {
        double sumAccelX = 0.0, sumAccelY = 0.0, sumAccelZ = 0.0;
        double sumGyroX = 0.0, sumGyroY = 0.0, sumGyroZ = 0.0;
        s64 firstTimestampMs = 0;
        u16 i;

        result = eMPU6500_IIO_RETURN_OK;

        for(i = 0; (i < numSamples) && (result == eMPU6500_IIO_RETURN_OK); i++)
        {
            mpu6500Sample_t singleSample;

            result = Mpu6500Iio_ReadSample(&singleSample);

            if(result == eMPU6500_IIO_RETURN_OK)
            {
                if(i == 0)
                {
                    firstTimestampMs = singleSample.timestampMs;
                }

                sumAccelX += (double)singleSample.accelX;
                sumAccelY += (double)singleSample.accelY;
                sumAccelZ += (double)singleSample.accelZ;
                sumGyroX  += (double)singleSample.gyroX;
                sumGyroY  += (double)singleSample.gyroY;
                sumGyroZ  += (double)singleSample.gyroZ;

                if((i + 1) < numSamples)
                {
                    Mpu6500Iio_SleepMs(sampleIntervalMs);
                }
            }
        }

        if(result == eMPU6500_IIO_RETURN_OK)
        {
            sampleOut->timestampMs = firstTimestampMs;
            sampleOut->accelX = (float)(sumAccelX / (double)numSamples);
            sampleOut->accelY = (float)(sumAccelY / (double)numSamples);
            sampleOut->accelZ = (float)(sumAccelZ / (double)numSamples);
            sampleOut->gyroX  = (float)(sumGyroX  / (double)numSamples);
            sampleOut->gyroY  = (float)(sumGyroY  / (double)numSamples);
            sampleOut->gyroZ  = (float)(sumGyroZ  / (double)numSamples);
        }
    }
    return result;
}

/**************************************
/** @brief Calibra os offsets de bias do sensor em software, assumindo que
           ele esta PARADO e sobre uma superficie nivelada durante a
           chamada. Detecta automaticamente qual eixo do acelerometro esta
           apontado para cima.
* @param numSamples: quantidade de amostras cruas usadas para calcular a
                      media de cada eixo. Sugestao de uso: 100 a 300.
* @retval eMPU6500_IIO_RETURN_OK se a calibracao foi concluida.
          eMPU6500_IIO_RETURN_INVALID_ARGUMENT se numSamples for zero.
          eMPU6500_IIO_RETURN_NOT_INITIALIZED se Init() ainda nao foi chamada.
          eMPU6500_IIO_RETURN_READ_ERROR se qualquer leitura falhar.
* @details O giroscopio, parado, deveria ler 0 rad/s em todos os eixos:
           qualquer media diferente de zero e tratada como bias e
           subtraida. O acelerometro, parado, deveria ler 0 no plano
           horizontal e +-1g no eixo vertical: esta funcao identifica o
           eixo vertical pela magnitude da media (o maior em modulo) e
           calibra os outros dois para zero.
**************************************/
mpu6500IioReturn_t Mpu6500Iio_Calibrate(u16 numSamples)
{
    mpu6500IioReturn_t result = eMPU6500_IIO_RETURN_INVALID_ARGUMENT;

    if(numSamples == 0)
    {
        result = eMPU6500_IIO_RETURN_INVALID_ARGUMENT;
    }
    else if(mpu6500Iio.isInitialized == false)
    {
        result = eMPU6500_IIO_RETURN_NOT_INITIALIZED;
    }
    else
    {
        double sumAccelRaw[3] = {0.0, 0.0, 0.0};
        double sumGyroRaw[3]  = {0.0, 0.0, 0.0};
        const char accelAxisLetters[3] = {'x', 'y', 'z'};
        const char gyroAxisLetters[3]  = {'x', 'y', 'z'};
        u16 i;
        u8 axisIndex;

        result = eMPU6500_IIO_RETURN_OK;

        for(i = 0; (i < numSamples) && (result == eMPU6500_IIO_RETURN_OK); i++)
        {
            for(axisIndex = 0; (axisIndex < 3) && (result == eMPU6500_IIO_RETURN_OK); axisIndex++)
            {
                s32 rawValue = 0;

                result = Mpu6500Iio_ReadRawAxis(dMPU6500_IIO_CHANNEL_ACCEL, accelAxisLetters[axisIndex], &rawValue);

                if(result == eMPU6500_IIO_RETURN_OK)
                {
                    sumAccelRaw[axisIndex] += (double)rawValue;
                }
            }

            for(axisIndex = 0; (axisIndex < 3) && (result == eMPU6500_IIO_RETURN_OK); axisIndex++)
            {
                s32 rawValue = 0;

                result = Mpu6500Iio_ReadRawAxis(dMPU6500_IIO_CHANNEL_GYRO, gyroAxisLetters[axisIndex], &rawValue);

                if(result == eMPU6500_IIO_RETURN_OK)
                {
                    sumGyroRaw[axisIndex] += (double)rawValue;
                }
            }

            if((i + 1) < numSamples)
            {
                Mpu6500Iio_SleepMs(dMPU6500_IIO_CALIB_SAMPLE_INTERVAL_MS);
            }
        }

        if(result == eMPU6500_IIO_RETURN_OK)
        {
            double meanAccelRaw[3];
            double oneGInRaw = dMPU6500_IIO_STANDARD_GRAVITY_MPS2 / mpu6500Iio.accelScale;
            u8 verticalAxis = 0;
            double largestMagnitude = 0.0;

            for(axisIndex = 0; axisIndex < 3; axisIndex++)
            {
                meanAccelRaw[axisIndex] = sumAccelRaw[axisIndex] / (double)numSamples;

                if(fabs(meanAccelRaw[axisIndex]) > largestMagnitude)
                {
                    largestMagnitude = fabs(meanAccelRaw[axisIndex]);
                    verticalAxis = axisIndex;
                }
            }

            for(axisIndex = 0; axisIndex < 3; axisIndex++)
            {
                double targetRaw = 0.0;
                s32 offset;

                if(axisIndex == verticalAxis)
                {
                    targetRaw = (meanAccelRaw[axisIndex] >= 0.0) ? oneGInRaw : -oneGInRaw;
                }

                offset = (s32)lround(targetRaw - meanAccelRaw[axisIndex]);

                if(axisIndex == 0) { mpu6500Iio.accelOffsetX = offset; }
                else if(axisIndex == 1) { mpu6500Iio.accelOffsetY = offset; }
                else { mpu6500Iio.accelOffsetZ = offset; }
            }

            mpu6500Iio.gyroOffsetX = (s32)lround(-(sumGyroRaw[0] / (double)numSamples));
            mpu6500Iio.gyroOffsetY = (s32)lround(-(sumGyroRaw[1] / (double)numSamples));
            mpu6500Iio.gyroOffsetZ = (s32)lround(-(sumGyroRaw[2] / (double)numSamples));

            mpu6500Iio.isCalibrated = true;
        }
    }
    return result;
}

/**************************************
/** @brief Informa se o sensor ja foi calibrado nesta execucao (por
           Calibrate() ou LoadCalibration()).
* @param isCalibrated: ponteiro onde o estado sera escrito.
* @retval eMPU6500_IIO_RETURN_OK se a leitura foi realizada.
          eMPU6500_IIO_RETURN_INVALID_ARGUMENT se isCalibrated for dNULL.
**************************************/
mpu6500IioReturn_t Mpu6500Iio_IsCalibrated(bool *isCalibrated)
{
    mpu6500IioReturn_t result = eMPU6500_IIO_RETURN_INVALID_ARGUMENT;

    if(isCalibrated != dNULL)
    {
        *isCalibrated = mpu6500Iio.isCalibrated;
        result = eMPU6500_IIO_RETURN_OK;
    }
    return result;
}

/**************************************
/** @brief Salva os offsets de calibracao atuais em um arquivo de texto
           simples, para serem recarregados em execucoes futuras sem
           precisar recalibrar (o que exigiria manter o sensor parado).
* @param filePath: caminho do arquivo a ser escrito (sera sobrescrito).
* @retval eMPU6500_IIO_RETURN_OK se salvo com sucesso.
          eMPU6500_IIO_RETURN_INVALID_ARGUMENT se filePath for dNULL.
          eMPU6500_IIO_RETURN_NOT_INITIALIZED se nenhuma calibracao foi
          feita ainda (nada a salvar).
          eMPU6500_IIO_RETURN_FILE_ERROR se o arquivo nao puder ser aberto.
**************************************/
mpu6500IioReturn_t Mpu6500Iio_SaveCalibration(const char *filePath)
{
    mpu6500IioReturn_t result = eMPU6500_IIO_RETURN_INVALID_ARGUMENT;

    if(filePath == dNULL)
    {
        result = eMPU6500_IIO_RETURN_INVALID_ARGUMENT;
    }
    else if(mpu6500Iio.isCalibrated == false)
    {
        result = eMPU6500_IIO_RETURN_NOT_INITIALIZED;
    }
    else
    {
        FILE *file = fopen(filePath, "w");

        if(file == dNULL)
        {
            result = eMPU6500_IIO_RETURN_FILE_ERROR;
        }
        else
        {
            fprintf(file, dMPU6500_IIO_CALIB_FILE_FORMAT,
                    (long)mpu6500Iio.accelOffsetX, (long)mpu6500Iio.accelOffsetY, (long)mpu6500Iio.accelOffsetZ,
                    (long)mpu6500Iio.gyroOffsetX, (long)mpu6500Iio.gyroOffsetY, (long)mpu6500Iio.gyroOffsetZ);
            fclose(file);
            result = eMPU6500_IIO_RETURN_OK;
        }
    }
    return result;
}

/**************************************
/** @brief Carrega offsets de calibracao de um arquivo salvo anteriormente
           por Mpu6500Iio_SaveCalibration(), evitando a necessidade de
           recalibrar a cada execucao.
* @param filePath: caminho do arquivo a ser lido.
* @retval eMPU6500_IIO_RETURN_OK se carregado com sucesso.
          eMPU6500_IIO_RETURN_INVALID_ARGUMENT se filePath for dNULL.
          eMPU6500_IIO_RETURN_NOT_INITIALIZED se Init() ainda nao foi chamada.
          eMPU6500_IIO_RETURN_FILE_ERROR se o arquivo nao existir ou o
          conteudo nao puder ser interpretado.
* @details Nao valida se a calibracao carregada ainda corresponde ao
           sensor fisico atual (ex.: se a placa foi trocada). Essa
           responsabilidade e de quem utiliza a biblioteca.
**************************************/
mpu6500IioReturn_t Mpu6500Iio_LoadCalibration(const char *filePath)
{
    mpu6500IioReturn_t result = eMPU6500_IIO_RETURN_INVALID_ARGUMENT;

    if(filePath == dNULL)
    {
        result = eMPU6500_IIO_RETURN_INVALID_ARGUMENT;
    }
    else if(mpu6500Iio.isInitialized == false)
    {
        result = eMPU6500_IIO_RETURN_NOT_INITIALIZED;
    }
    else
    {
        FILE *file = fopen(filePath, "r");

        if(file == dNULL)
        {
            result = eMPU6500_IIO_RETURN_FILE_ERROR;
        }
        else
        {
            long accelX, accelY, accelZ, gyroX, gyroY, gyroZ;
            int itemsRead = fscanf(file, dMPU6500_IIO_CALIB_FILE_FORMAT,
                                    &accelX, &accelY, &accelZ, &gyroX, &gyroY, &gyroZ);
            fclose(file);

            if(itemsRead == 6)
            {
                mpu6500Iio.accelOffsetX = (s32)accelX;
                mpu6500Iio.accelOffsetY = (s32)accelY;
                mpu6500Iio.accelOffsetZ = (s32)accelZ;
                mpu6500Iio.gyroOffsetX  = (s32)gyroX;
                mpu6500Iio.gyroOffsetY  = (s32)gyroY;
                mpu6500Iio.gyroOffsetZ  = (s32)gyroZ;
                mpu6500Iio.isCalibrated = true;
                result = eMPU6500_IIO_RETURN_OK;
            }
            else
            {
                result = eMPU6500_IIO_RETURN_FILE_ERROR;
            }
        }
    }
    return result;
}

/**************************************
* FUNCOES LOCAIS
**************************************/
/**************************************
/** @brief Localiza o dispositivo IIO do sensor, tentando primeiro o nome
           exato do chip e, se configurado, um nome de fallback.
* @param Nenhum.
* @retval eIIO_SYSFS_RETURN_OK se algum dos nomes configurados foi
          encontrado. Qualquer outro retorno de IioSysfs_FindDevice() caso
          contrario.
**************************************/
static iioSysfsReturn_t Mpu6500Iio_LocateDevice(void)
{
    iioSysfsReturn_t result = IioSysfs_FindDevice(dMPU6500_IIO_PRIMARY_NAME, mpu6500Iio.devPath, sizeof(mpu6500Iio.devPath));

    #if dMPU6500_IIO_TRY_FALLBACK_NAME == dTRUE
        if(result != eIIO_SYSFS_RETURN_OK)
        {
            result = IioSysfs_FindDevice(dMPU6500_IIO_FALLBACK_NAME, mpu6500Iio.devPath, sizeof(mpu6500Iio.devPath));
        }
    #endif

    return result;
}

/**************************************
/** @brief Le o valor "raw" (bruto, sem calibracao nem escala) de um eixo.
* @param channelType: nome do canal IIO ("accel" ou "anglvel").
* @param axis: letra do eixo ('x', 'y' ou 'z').
* @param rawOut: ponteiro onde o valor bruto sera escrito.
* @retval eMPU6500_IIO_RETURN_OK se a leitura for bem sucedida.
          eMPU6500_IIO_RETURN_READ_ERROR caso contrario.
**************************************/
static mpu6500IioReturn_t Mpu6500Iio_ReadRawAxis(const char *channelType, char axis, s32 *rawOut)
{
    mpu6500IioReturn_t result = eMPU6500_IIO_RETURN_READ_ERROR;
    char attrName[dMPU6500_IIO_ATTR_NAME_MAX];

    snprintf(attrName, sizeof(attrName), "in_%s_%c_raw", channelType, axis);

    if(IioSysfs_ReadAttrLong(mpu6500Iio.devPath, attrName, rawOut) == eIIO_SYSFS_RETURN_OK)
    {
        result = eMPU6500_IIO_RETURN_OK;
    }
    return result;
}

/**************************************
/** @brief Le o valor raw de um eixo e converte para unidade fisica,
           aplicando o offset de calibracao em software (sempre, mesmo que
           seja zero por ainda nao ter sido calibrado).
* @param channelType: nome do canal IIO ("accel" ou "anglvel").
* @param axis: letra do eixo ('x', 'y' ou 'z').
* @param scale: fator de conversao raw -> unidade fisica.
* @param offset: offset de calibracao (em unidades de raw) deste eixo.
* @param out: ponteiro onde o valor convertido sera escrito.
* @retval eMPU6500_IIO_RETURN_OK se a leitura for bem sucedida.
          eMPU6500_IIO_RETURN_READ_ERROR caso contrario.
**************************************/
static mpu6500IioReturn_t Mpu6500Iio_ReadCalibratedAxis(const char *channelType, char axis, double scale, s32 offset, float *out)
{
    mpu6500IioReturn_t result;
    s32 rawValue = 0;

    result = Mpu6500Iio_ReadRawAxis(channelType, axis, &rawValue);

    if(result == eMPU6500_IIO_RETURN_OK)
    {
        double physicalValue = ((double)rawValue + (double)offset) * scale;
        *out = (float)physicalValue;
    }
    return result;
}

/**************************************
/** @brief Retorna o instante atual em milissegundos desde a epoca Unix (UTC).
* @param Nenhum.
* @retval Timestamp atual, em milissegundos.
**************************************/
static s64 Mpu6500Iio_GetTimestampMs(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_REALTIME, &ts);

    return ((s64)ts.tv_sec * 1000) + (ts.tv_nsec / 1000000);
}

/**************************************
/** @brief Suspende a execucao da thread atual pelo tempo informado.
* @param milliseconds: tempo a aguardar, em milissegundos.
* @retval Nenhum.
**************************************/
static void Mpu6500Iio_SleepMs(u16 milliseconds)
{
    struct timespec ts;

    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (long)(milliseconds % 1000) * 1000000L;

    nanosleep(&ts, dNULL);
}
/** @} DOXYGEN GROUP TAG END OF FILE */
