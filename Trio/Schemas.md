├── schema/
│   ├── bmp_raw.json
│   ├── mpu_raw.json
│   ├── bmp_mpu.json
│   └── final.json

### Tópicos 

| Topic          | Payload | QoS    | Retain | Publisher | Subscriber  |
| -------------- | ------- | ------ | ------ | --------- | ----------- |
| sensor/bmp/raw |         | 0      |        | Nó 1      | Nó 2        |
| sensor/mpu/raw |         | 0      |        | Nó 2      | -           |
| fusion/bmp_mpu |         | 1 ou 2 | Sim    | Nó 2      | Nó 3        |
| fusion/final   |         | ?      |        | No 3      | Interface   |
| status/rasp1   |         | 2      | s      | Nó 1      | Nó 2 / Nó 3 |
| status/rasp2   |         | 2      | s      | Nó 2      | Nó 1 / Nó 3 |
| status/rasp3   |         | 2      | s      | Nó 3      | Nó 1 / Nó 2 |
```
Topic: sensor/bmp/raw

Campos:
    timestamp → int64, Unix milliseconds, UTC
    temperature → float, °C
    pressure → float, Pa
    altitude → float, m
```

```
Topic: sensor/mpu/raw

Campos:
	timestamp → int64, Unix milliseconds, UTC
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
Topic: fusion/bmp_mpu

Campos:
	timestamp (fusion) → int64, Unix milliseconds, UTC
	
	bmp → object
		timestamp (bmp) → int64, Unix milliseconds, UTC
	    temperature → float, °C
	    pressure → float, Pa
	    altitude → float, m
	
	mpu -> object 
		timestamp → int64, Unix milliseconds, UTC
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
	    temperature → float, °C
	    pressure → float, Pa
	    altitude → float, m
	
	mpu -> object 
		timestamp → int64, Unix milliseconds, UTC
		velocidade → number, m/s²
		direção → string 
```

```
Topic: status/rasp1

Campos:
    timestamp → int64, Unix milliseconds, UTC
    Status → string, "ON" / "OFF"
```

```
Topic: status/rasp1

Campos:
    timestamp → int64, Unix milliseconds, UTC
    Status → string, "ON" / "OFF"
```

```
Topic: status/rasp2

Campos:
    timestamp → int64, Unix milliseconds, UTC
    Status → string, "ON" / "OFF"
```

```
Topic: status/rasp3

Campos:
    timestamp → int64, Unix milliseconds, UTC
    Status → string, "ON" / "OFF"
```
*Obs: timestamp indica o tempo de leitura / coleta

#### Exemplos json: 
BMP:
{
    "timestamp": 1789335000123,
    "temperature": 25.43,
    "pressure": 101325.2,
    "altitude": 12.7
}

MPU:
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

fusion/bmp_mpu 
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


### cJSON 

cJSON types:
`String  → cJSON_AddStringToObject()`
`Number  → cJSON_AddNumberToObject()`
`Boolean → cJSON_AddBoolToObject()`
`Object  → cJSON_AddObjectToObject()`
`Array   → cJSON_AddArrayToObject()`