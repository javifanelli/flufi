# 👨‍👩‍👧 La familia animada digital

Pitri y Flufi son de la misma familia, ¡y se hablan! Se saludan, charlan, juegan juntos, se
consuelan, se mandan besos y se cuentan cosas.

Este archivo es **igual** en todos los personajes de la familia.

---

## 📡 Como se hablan

Se mandan **mensajitos por la red WiFi de la casa**. Es como si se pasaran papelitos:
cada papelito dice **quien lo manda**, **para quien es** y **que es** (un saludo, un beso…).

- Los mensajes no salen de la casa: no pasan por internet.
- Todos los papelitos llevan una **palabra secreta** que solo conoce la familia. Si llega uno
  sin la palabra, se ignora: asi ningun otro aparato se puede hacer pasar por Pitri o Flufi.
- A veces un papelito se pierde en el camino. Por eso cada uno se manda **3 veces**, con un
  **numero**: si llega repetido, el que lo recibe se da cuenta y le hace caso una sola vez.
- Cada tanto, cada personaje avisa "¡estoy aca!": asi todos saben quien esta conectado.

## 💬 Lo que hacen juntos

| Que pasa | Que hacen |
|---|---|
| Uno se conecta al WiFi | El otro lo saluda: "¡Hola Flufi!" / "¡Hola Pitri!" |
| Cada 4 a 10 minutos | Uno **empieza una charla** ("¿Como estas?" / "¡Bien! ¿Y vos?"…): cada uno dice lo suyo en su pantalla, por turnos. O proponen **jugar juntos** y hacen el mismo juego a la vez (se marean, se ponen contentos, se ponen los lentes…) |
| Uno se pone a llorar | El otro lo **consuela** ("¡No llores!") y le manda un beso |
| El sensor de Pitri ve a alguien | Flufi tambien saluda: "¡Llego alguien!" |
| A Pitri le llega un mensaje de Telegram | Flufi avisa: "¡Pitri tiene un mensaje!" |
| Pitri se duerme | A Flufi le da sueño y dice "Pitri se durmio" |
| Flufi cuenta el tiempo de afuera | Se lo pasa a Pitri, que no tiene pronostico: "Afuera: Soleado 28°C" |
| Pitri mira su sensor | Le pasa la humedad a Flufi, que no la mide |

Y desde el menu, en **Familia**, se elige a alguien y que hacerle: **Beso** (se le ponen los
ojos de corazon), **Cosquis** (se rie), **Convidar** (come algo rico y dice gracias), **Charlar**
o **Jugar**.

---

## 🧑‍🔧 Para grandes

### Configurar

1. En cada personaje, copiar `components/familia/familia_secretos.ejemplo.h` como
   `components/familia/familia_secretos.h`.
2. Poner **la misma** `FAMILIA_CLAVE` en todos (sin espacios, hasta 31 letras).
3. Todos tienen que estar en la **misma red WiFi**.

`familia_secretos.h` esta en el `.gitignore` (`*secretos*.h`): nunca se sube.

### El protocolo (version 2)

- **UDP**, puerto **47100**. Un mensaje = un datagrama = una linea de texto ASCII:

  ```
  FAMILIA <version> <clave> <de> <para> <numero> <TIPO> [dato]
  ```

  | Campo | Que es |
  |---|---|
  | `FAMILIA` | Marca fija: si no empieza asi, no es para nosotros |
  | `<version>` | Version del protocolo (hoy `2`). Si no coincide, se ignora y se avisa en el monitor |
  | `<clave>` | La palabra secreta (`FAMILIA_CLAVE`) |
  | `<de>` | Nombre del que lo manda (sin espacios, hasta 15 letras) |
  | `<para>` | Nombre del destinatario, o `*` para todos |
  | `<numero>` | Numero del mensaje (sube de a uno; arranca al azar al prender) |
  | `<TIPO>` | Ver la tabla de abajo |
  | `[dato]` | Opcional: el resto de la linea (hasta 47 letras) |

  Ejemplo: `FAMILIA 2 palabrasecreta Flufi Pitri 52811 CHARLA 3 1`

- Cada mensaje se manda **3 veces** (40 ms entre una y otra). El receptor recuerda los ultimos 8
  numeros de cada personaje y descarta los repetidos. `ESTOY` se manda una sola vez.
- Los mensajes para todos (`*`) van por **broadcast** (`255.255.255.255`). Los que van para uno
  solo, directo a su IP si ya se la conoce; si no, por broadcast.
- Al conectarse, cada personaje desactiva el ahorro de energia del WiFi (`WIFI_PS_NONE`): con
  el ahorro, la placa se pierde muchos broadcasts.
- Al conectarse manda `HOLA` a todos, y despues `ESTOY` cada 30 s. Si de alguien no se escucha
  nada en 95 s, se lo da por desconectado.
- `HOLA` se contesta con `SALUDO`. Si se escucha por primera vez a alguien con otro mensaje
  (su `HOLA` se perdio), igual se lo saluda y se le manda `SALUDO`. Para no saludar dos veces
  seguidas al mismo, despues de saludarlo se esperan 60 s.
- **Compatibilidad hacia adelante:** si llega un `TIPO` desconocido, se ignora (y se avisa en
  el monitor). Asi un personaje nuevo puede inventar mensajes sin romper a los viejos.
- **Seguridad:** la clave viaja en texto plano dentro de la red de la casa. Alcanza para que no
  se cuelen mensajes por accidente o de otros aparatos, pero no es cifrado.

### Los tipos de mensaje

| `TIPO` | `dato` | Lo manda | El que lo recibe |
|---|---|---|---|
| `HOLA` | — | Cualquiera, al conectarse (a todos) | Saluda y contesta `SALUDO` |
| `SALUDO` | — | Respuesta a `HOLA` | Saluda |
| `ESTOY` | — | Cualquiera, cada 30 s (a todos) | Solo anota que esta conectado |
| `BESO` | — | Menu Familia, o al consolar | Ojos de corazon y "¡Un beso de …!" |
| `CHARLA` | `<charla> <renglon>` | El que tiene el turno en una charla | Dice ese renglon y, si sigue, le devuelve el turno con `<renglon + 1>` |
| `VISITA` | — | Quien tiene sensor de movimiento, al ver a alguien (cada 2 min como mucho) | Saluda: "¡Llego alguien!" |
| `COMIDA` | — | Menu Familia (Convidar) | "¡Gracias …!" y come una de sus comidas |
| `COSQUILLAS` | — | Menu Familia (Cosquis) | Se pone contento y "¡Jaja! ¡Basta!" |
| `JUGAR` | `<juego>` | Cada tanto, o menu Familia (Jugar) | Hace ese juego (el que lo manda hace el mismo a la vez) |
| `TRISTE` | — | El que acaba de llorar (a todos) | "¡No llores!" y le manda `BESO` |
| `CLIMA` | `<temp. en decimas> <cielo>` | Quien tiene pronostico (Flufi), en x:00 y x:30 | "Afuera: <cielo> <temp>°C" |
| `SENSOR` | `<temp. en decimas> <humedad %>` | Quien tiene sensor de humedad (Pitri), en x:00/15/30/45 | "<de>: Humedad <h>%" |
| `MENSAJE` | — | Quien recibe Telegram (Pitri), al llegar uno | "¡<de> tiene un mensaje!" |
| `DORMIR` | — | El que se va a dormir | Bosteza (juego `sueno`) y "<de> se durmio" |

Las **charlas** y los **juegos para hacer juntos** estan en `components/familia/charlas.c`
(iguales en todos). Los juegos de esa lista tienen que existir, con ese nombre, en todos los
personajes (`eyes_jugar_juego()`).

### En el codigo

- El componente `components/familia` es **identico** en todos los personajes: si se cambia,
  copiarlo a todos.
- `familia_iniciar(nombre)`: arranca la tarea (espera WiFi, manda `HOLA`, escucha).
- `familia_proximo_evento(&e)`: lo que le dijeron (lo atiende `main.c`, en `atender_familia()`).
- `familia_mandar(para, tipo, dato)`: mandarle algo a alguien (`"*"` = a todos).
- `familia_conectados()` / `familia_alguien_al_azar()`: quienes estan conectados.
- `familia_charla()` / `familia_juego_junto()`: las charlas y los juegos compartidos.
- En `main.c`: `atender_familia()` (reacciones), `iniciativa_familia()` (charlas y juegos por su
  cuenta, cada `FAMILIA_INICIATIVA_MIN_S` a `FAMILIA_INICIATIVA_MAX_S`) y `menu_familia()`.
- Mientras un personaje esta contestando (o charlando), lo que llega espera su turno; si no,
  un mensaje nuevo corta lo que esta haciendo para contestar enseguida.

### Sumar un tipo de mensaje nuevo

1. Agregarlo al final de `familia_tipo_t` en `familia.h` y a la lista `palabras` en `familia.c`.
2. Copiar el componente a todos los personajes.
3. En el `main.c` de cada personaje, que reaccione en `atender_familia()`.
4. Anotarlo en la tabla de este archivo (en todos los personajes).

### Sumar un personaje nuevo a la familia

1. Copiar `components/familia` (con su `familia_secretos.h`, la misma clave).
2. Llamar a `familia_iniciar("SuNombre")` despues de `wifi_iniciar()`, y copiar de otro
   personaje `atender_familia()`, `iniciativa_familia()` y `menu_familia()`.
3. Que tenga los juegos de la lista de juegos compartidos (`charlas.c`).
4. Copiar este archivo a sus `docs/`.
