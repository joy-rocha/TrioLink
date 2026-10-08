├── schema/
│   ├── bmp_raw.json
│   ├── mpu_raw.json
│   ├── bmp_mpu.json
│   └── final.json

### Tópicos 

| Topic          | QoS | Retain | Publisher | Subscriber  |
| -------------- | --- | ------ | --------- | ----------- |
| sensor/bmp/raw | 0   | Não    | Nó 1      | Nó 2        |
| sensor/mpu/raw | 0   | Não    | Nó 2      | -           |
| fusion/bmp_mpu | 1   | Sim    | Nó 2      | Nó 3        |
| fusion/final   | 1   | Sim    | No 3      | Interface   |
| status/rasp1   | 1   | Sim    | Nó 1      | Nó 2 / Nó 3 |
| status/rasp2   | 1   | Sim    | Nó 2      | Nó 1 / Nó 3 |
| status/rasp3   | 1   | Sim    | Nó 3      | Nó 1 / Nó 2 |

### Regra de staleness (dado obsoleto)

Cada nó consumidor mantém, para cada tópico que assina, o timestamp da última mensagem válida recebida.

- **Threshold de staleness**: se `(agora - timestamp_da_ultima_mensagem_recebida) > 3x o intervalo de publicação esperado do tópico`, o dado é considerado obsoleto. O intervalo de publicação esperado de cada tópico deve ser combinado entre os três (ex: se `sensor/bmp/raw` publica a cada 1s, o threshold de staleness seria 3s).
- **Quando um dado fica obsoleto, o nó consumidor NÃO trava e NÃO omite o campo.** Ele continua publicando sua própria saída normalmente, reaproveitando o último valor conhecido daquele campo, mas:
    - marca o campo `stale` correspondente como `true`
    - mantém o `timestamp` original daquele sub-objeto (não atualiza para "agora") — isso preserva a informação de quando o dado realmente foi lido pela última vez
- Quando a mensagem volta a chegar normalmente, o nó volta a marcar `stale: false` e atualiza os valores/timestamp normalmente.
- O shape do JSON nunca muda entre estado normal e estado obsoleto — só o valor de `stale` e o quão "velho" o `timestamp` está.

**Quando o `system_status` muda (`fusion/final`):**
- `"OK"`: nenhum dos dois campos de staleness ativos (`bmp.stale == false` E `mpu_stale == false`).
- `"INVALID"`: `bmp.stale == true` OU `mpu_stale == true` — ou seja, se qualquer entrada crítica estiver obsoleta, o estado consolidado já não é considerado confiável, mesmo que o Nó 3 continue publicando o último valor calculado.

**Origem de cada flag de staleness:**
- `bmp.stale` (em `fusion/bmp_mpu` e `fusion/final`): setado pelo Nó 2, quando `sensor/bmp/raw` (Nó 1) para de chegar dentro do threshold. Propagado adiante sem recálculo pelo Nó 3.
- `mpu_stale` (em `fusion/final`): setado pelo Nó 3, quando `fusion/bmp_mpu` (Nó 2) para de chegar dentro do threshold — ou seja, indica que o **Nó 2 inteiro** (não só o sensor MPU) parou de responder. Pode ser cruzado com `status/rasp2 == "OFFLINE"` para confirmação adicional, mas o timestamp já é suficiente para decidir sozinho.

```
Topic: sensor/bmp/raw

Campos:
    timestamp → int64 
	    Unix milliseconds, UTC
	    - Momento da aquisição da leitura.
	    
    temperature → number (float)
	    Unidade: °C
	    
    pressure → number (float)
	    Unidade: Pa
	    
    altitude → number (float)
	    Unidade: m
```

```
Topic: sensor/mpu/raw

Campos:
	timestamp → int64 
	    Unix milliseconds, UTC
	    - Momento da aquisição da leitura.
	    
	accel → object 
		x → float, Unidade: m/s²
		y → float, Unidade: m/s² 
		z → float, Unidade: m/s² 
		
	gyro → object 
		x → float, Unidade: rad/s 
		y → float, Unidade: rad/s 
		z → float, Unidade: rad/s
```

```
Topic: fusion/bmp_mpu

Campos:
	timestamp → int64 
	    Unix milliseconds, UTC
	    - Momento em que o Nó 2 gerou esta mensagem de fusão.
	
	bmp → object
		timestamp (bmp) → int64, Unix milliseconds, UTC
		    - Momento da última leitura VÁLIDA recebida de sensor/bmp/raw
		    (não atualiza enquanto stale == true)
	    temperature → float, °C
	    pressure → float, Pa
	    altitude → float, m
	    stale → boolean
	        - true se sensor/bmp/raw não chegou dentro do threshold
	        de staleness; valores acima são o último conhecido.
	
	mpu -> object 
		timestamp (mpu) → int64, Unix milliseconds, UTC
		    - Sempre a leitura atual: o próprio Nó 2 gera esse dado,
		    então não fica obsoleto dentro deste tópico.
		
		accel → object 
			x → float, m/s²
			y → float, m/s² 
			z → float, m/s² 
			
		gyro → object 
			x → float, rad/s 
			y → float, rad/s 
			z → float, rad/s
			
```

```
Topic: fusion/final

Campos:
	timestamp (fusion) → int64, Unix milliseconds, UTC
	
	bmp → object
		timestamp (bmp) → int64, Unix milliseconds, UTC
		    - Repassado de fusion/bmp_mpu.bmp.timestamp, sem recálculo.
	    temperature → float, °C
	    pressure → float, Pa
	    altitude → float, m
	    stale → boolean
	        - Repassado de fusion/bmp_mpu.bmp.stale, sem recálculo.
	
	mpu -> object 
		timestamp → int64, Unix milliseconds, UTC
		velocidade → number, m/s
		direção → string
		    - Valores válidos: "cima", "baixo", "esquerda",
		    "direita", "frente", "trás"
		
	mpu_stale → boolean
	    - true se fusion/bmp_mpu (Nó 2) não chegou dentro do
	    threshold de staleness. Refere-se ao Nó 2 como um todo,
	    não só ao sensor BMP repassado por ele.
		
	system_status → String
		valores: "OK", "INVALID"
		- "OK": bmp.stale == false E mpu_stale == false
		- "INVALID": bmp.stale == true OU mpu_stale == true
```

```
Topic: status/rasp1

Campos:
    timestamp → int64 
	    Unix milliseconds, UTC
    
    Status → string
	    Valores: "ONLINE" / "OFFLINE"
```

```
Topic: status/rasp2

Campos:
    timestamp → int64 
	    Unix milliseconds, UTC
    
    Status → string
	    Valores: "ONLINE" / "OFFLINE"
```

```
Topic: status/rasp3

Campos:
    timestamp → int64 
	    Unix milliseconds, UTC
    
    Status → string
	    Valores: "ONLINE" / "OFFLINE"
```
*Obs: timestamp indica o tempo de leitura / coleta, NÃO INDICA ENVIO

#### Exemplos json: 

BMP -> 
```json
{
    "timestamp": 1789335000123,
    "temperature": 25.43,
    "pressure": 101325.2,
    "altitude": 12.7
}
```

MPU -> 
```json
{
    "timestamp": 1789335000123,

    "accel": {
        "x": 0.12,
        "y": -0.03,
        "z": 9.78
    },

    "gyro": {
        "x": 0.01,
        "y": -0.02,
        "z": 0.04
    }
}
```

fusion/bmp_mpu (caso normal) -> 
```json
{
    "timestamp": 1789335000123,

    "bmp": {
        "timestamp": 1789335000123,
        "temperature": 25.43,
        "pressure": 101325.2,
        "altitude": 12.7,
        "stale": false
    },

    "mpu": {
        "timestamp": 1789335000123,

        "accel": {
            "x": 0.12,
            "y": -0.03,
            "z": 9.78
        },

        "gyro": {
            "x": 0.01,
            "y": -0.02,
            "z": 0.04
        }
    }
}
```

fusion/bmp_mpu (Nó 1 parou de publicar, reaproveitando último bmp conhecido) -> 
```json
{
    "timestamp": 1789335009123,

    "bmp": {
        "timestamp": 1789335000123,
        "temperature": 25.43,
        "pressure": 101325.2,
        "altitude": 12.7,
        "stale": true
    },

    "mpu": {
        "timestamp": 1789335009123,

        "accel": {
            "x": 0.15,
            "y": -0.02,
            "z": 9.79
        },

        "gyro": {
            "x": 0.02,
            "y": -0.01,
            "z": 0.03
        }
    }
}
```

fusion/final -> 
```json
{
    "timestamp": 1789335000123,

    "bmp": {
        "timestamp": 1789335000123,
        "temperature": 25.43,
        "pressure": 101325.2,
        "altitude": 12.7,
        "stale": false
    },

    "mpu": {
        "timestamp": 1789335000123,
        "velocidade": 24.0,
        "direção": "cima"
    },

    "mpu_stale": false,

    "system_status": "OK"
}
```

status/# -> 
```json
{
    "timestamp": 1789335000123,
    "Status": "ONLINE"
}
```

### cJSON 

cJSON types:
`String  → cJSON_AddStringToObject()`
`Number  → cJSON_AddNumberToObject()`
`Boolean → cJSON_AddBoolToObject()`
`Object  → cJSON_AddObjectToObject()`
`Array   → cJSON_AddArrayToObject()`
