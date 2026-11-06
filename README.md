# 🩷 Flufi

**Flufi es la compañera de Pitri.** Tiene una carita en una pantalla a color, con ojitos
que parpadean y pestañas.

La armamos entre Sofia y su papa, de a poquito, como hicimos con Pitri.

```
        \ | /            \ | /
         \|/              \|/
       ████████        ████████
       ████████        ████████      ← los ojitos de Flufi, con pestañas
       ████████        ████████
       ████████        ████████
```

---

## ✨ Que sabe hacer Flufi

| Cuando… | Flufi… |
|---|---|
| Se prende | Dice **"¡Hola Sofi!"** y **"Soy Flufi"**, bien grande |
| Se esta conectando al WiFi | Abajo de sus ojitos aparecen unas **ondas** que se encienden de a una |
| Se conecta | Dice **"Tengo wifi!"** |
| Sabe la hora | Saluda segun el momento del dia: **"¡Buen dia!"**, **"¡Buenas tardes!"** o **"¡Buenas noches!"** |
| Pasa un ratito | Parpadea (a veces dos veces seguidas) |

¡Y vamos a ir enseñandole mas cosas!

---

## 🧩 De que esta hecha Flufi

Usa el mismo hardware del proyecto **color1.44**:

| Parte | Para que sirve |
|---|---|
| **ESP32-C3** (la placa) | Es el **cerebro**: una computadora chiquita que ejecuta el programa |
| **Pantalla TFT 1.44" a color** (128×128) | Es la **cara**: ahi se dibujan los ojos |
| **Sensor BMP280** | Siente la **temperatura** y la **presion del aire** (todavia no lo usamos) |
| **2 botones** | Todavia no los usamos |
| **LED de colores** de la placa | Por ahora lo dejamos apagado |

Las conexiones de cada cable estan en
[components/board_config/include/board_config.h](components/board_config/include/board_config.h).

---

## 🎛️ Cambia como es Flufi

En [main/flufi_config.h](main/flufi_config.h) se pueden cambiar los colores y el parpadeo:

```c
#define COLOR_OJOS      COLOR(120, 210, 255)   // celeste
#define COLOR_PESTANAS  COLOR(255, 120, 190)   // rosa
```

Cada color se arma con tres numeros de 0 a 255: **cuanto rojo, cuanto verde y cuanto
azul**. Por ejemplo, `COLOR(255, 0, 0)` es rojo, `COLOR(255, 255, 0)` es amarillo y
`COLOR(180, 100, 255)` es violeta.

Ahi tambien estan los mensajes del saludo, por ejemplo:

```c
#define MENSAJE_ARRANQUE        "¡Hola\nSofi!"
#define MENSAJE_ARRANQUE_2      "Soy\nFlufi"
```

Y en [components/eyes/eyes.c](components/eyes/eyes.c), arriba de todo, esta la forma de
los ojos (ancho, alto, que tan redondos son, que tan separados estan).

---

## 📶 WiFi

El nombre y la contraseña de la red WiFi son un **secreto de la casa**, asi que no se suben
al repo:

1. Copia `components/wifi/wifi_secretos.ejemplo.h` en la misma carpeta con el nombre
   `wifi_secretos.h`.
2. En la copia, completa `WIFI_NOMBRE_RED` y `WIFI_CONTRASENA`.

`wifi_secretos.h` esta en el `.gitignore`: se queda en la compu y nunca se sube. Si no esta,
Flufi funciona igual, sin WiFi. Con WiFi, Flufi saca la hora de internet (la zona horaria
esta en `main/flufi_config.h`).

---

## 🧑‍🔧 Compilar y cargar

```
idf.py build
idf.py -p COM8 flash monitor
```

---

## 📔 Diario

Lo que vamos haciendo esta en [docs/DIARIO.md](docs/DIARIO.md).
