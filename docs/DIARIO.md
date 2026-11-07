# 📔 Diario de Flufi

Aca anotamos todo lo que le vamos enseñando a Flufi y lo que aprendemos en el camino.
Lo mas nuevo va arriba.

> **Para anotar algo nuevo**, copia esto arriba de todo y completalo:
>
> ```
> ## AAAA-MM-DD — Titulo
>
> **Que hicimos:** …
> **Que aprendimos:** …
> ```

---

## 2026-11-06 — ¡Flufi cuenta el tiempo! ☀️🌧️

**Que hicimos:** Ahora Flufi sabe **como esta el tiempo afuera**. Cada media hora (en punto y a las y media) se aleja y cuenta si esta **soleado**, **nublado**, si **llueve**, si hay **tormenta** o si **nieva**, con un dibujo que se mueve: el sol con sus rayos girando, nubes, gotas que caen, un rayo que titila... Abajo dice la temperatura y si hace **frio** o **calor**. El LED tambien se pone del color del tiempo. Tambien se puede ver desde el menu, en **Clima**.

**Que aprendimos:**
- El tiempo lo sacamos de **Open-Meteo**, un servicio gratis: Flufi le pregunta por internet como esta el tiempo en nuestra zona y le contesta con numeros.
- El tiempo se describe con un **codigo mundial** (se llama WMO): 0 es despejado, 3 nublado, 61 lluvia, 95 tormenta... Asi todos los servicios del mundo hablan el mismo idioma.
- **Donde vivimos es un dato privado**, asi que va en un archivo secreto que no se sube, como la clave del WiFi. Y alcanza con la zona del barrio: el tiempo es igual en toda la cuadra.
- Le agrandamos a Flufi el lugar para el programa (de 1 MB a 1,5 MB), porque la conexion segura ocupa bastante.

---

## 2026-11-05 — Modos Fantasia y Ejercicio 🧚⚽

**Que hicimos:** Agregamos dos modos nuevos. En **Fantasia** Flufi hace de personajes de cuentos de hadas: **hada** (con alas y varita), **princesa** (con corona), **sirena** (¡el fondo se vuelve mar!), **dragoncita** (que tira fuego) y **caperucita** (con su capucha roja y su canasta). En **Ejercicio** hace deporte: **patina**, **baila** (con luces de colores), juega al **futbol** (¡gol!), anda en **bici** y levanta **pesas** contando las repeticiones. Todos tienen los ojos, las pestañas y los labios de Flufi, y casi todos sus rulos.

**Que aprendimos:**
- Ya tenemos 15 personajes, asi que los ordenamos: un archivo por modo (`animales.c`, `fantasia.c` y `ejercicio.c`) y uno con lo que comparten todos (`personajes.c`).
- Para la sirena cambiamos el **color del fondo** por un azul de mar. Todo lo que se "borra" con el color del fondo (como la rayita entre los labios) tambien se vuelve azul.
- Las ruedas de la bici y de los patines **giran** porque sus rayitas se dibujan en otro angulo en cada cuadro.

---

## 2026-11-05 — Nuevos colores y animales sin rulos 🎨

**Que hicimos:** Cambiamos el color del LED de dos emociones: cuando Flufi esta **tranquila** ahora es **rosa**, y cuando se **sorprende** es **celeste**. A la coneja le pusimos el LED **blanco**, como su pelaje, para que no se confunda con el rosa de tranquila. Y decidimos que **los animales no tienen rulos**: la mariposa tambien se los saca y se queda solo con sus antenitas.

**Que aprendimos:**
- Los colores del LED de cada emocion estan arriba de todo en , y los de los animales en la lista de . ¡Se pueden cambiar cuando quieran!

---

## 2026-11-05 — ¡Modo Animales! 🐰🦄🐭🦋🦊

**Que hicimos:** Sofia eligio cinco animales para que Flufi los interprete: **coneja**, **unicornio**, **ratoncita**, **mariposa** y **zorrita**. En el menu, en **Modo**, ahora se puede elegir entre **Normal** (sus emociones) y **Animales**. Cada animal mantiene los ojos, las pestañas y los labios de Flufi, y le agrega sus partes: orejas, cuerno, alas, bigotes... El unicornio cambia los rulos por una crin de arco iris, y la mariposa se queda con sus rulos y le salen antenitas. Para cambiar de animal, Flufi cierra los ojos y los abre ya disfrazada.

**Que aprendimos:**
- Las orejas de la coneja estan hechas con **muchos circulitos en fila**. Si la fila se tuerce por la mitad, ¡la oreja se dobla!
- Las alas de la mariposa son **ovalos** que se hacen mas finitos y mas anchos una y otra vez: asi parece que aletea.
- Pusimos los animales en un archivo aparte (`animales.c`), igual que los disfraces de Pitri, para que cada archivo sea mas facil de leer.

---

## 2026-11-05 — Una gaseosa con burbujas 🥤

**Que hicimos:** Cambiamos el mate por una **gaseosa**: una lata roja con una onda blanca, el anillito arriba y tres burbujitas que suben. Con cada sorbo se va una burbuja, y al ultimo sorbo Flufi dice **"Burp!"**.

**Que aprendimos:**
- La onda blanca de la lata es una **onda seno**: para cada columna, una cuenta dice cuanto sube o baja la rayita. Es la misma cuenta que usamos para la boca de mareada.

---

## 2026-11-05 — Los cables del sensor estaban cruzados 🔌

**Que hicimos:** El sensor BMP280 nunca habia andado: Flufi decia que no lo encontraba. Mirando el proyecto color1.44 (que usa el mismo hardware) vimos que los cables estan al reves de lo que pensabamos: **SDA va al GPIO 1 y SCL al GPIO 0**. Tambien dimos vuelta los botones A y B, que estaban cambiados.

**Que aprendimos:**
- En I2C hay dos cables: **SDA** lleva los datos y **SCL** marca el ritmo (el reloj). Si se cruzan, el sensor no entiende nada y no contesta. ¡A Pitri le paso algo parecido con las patas del sensor de temperatura!
- Cuando algo ya funcionaba en otro proyecto, conviene **copiar como estaba conectado ahi**.

---

## 2026-11-05 — El menu, y la cara que se aleja 🍩

**Que hicimos:** La hora tapaba la boca de Flufi, asi que ahora cuando quiere mostrar algo (la hora, el sensor, "Tengo wifi!") su **cara se aleja**: se achica, se corre a la izquierda y el texto entra deslizandose por la derecha. Y por fin Flufi tiene su **menu**, que se maneja con sus tres botones: **A** abre y elige, **B** pasa a la siguiente opcion y **BOOT** vuelve atras. Adentro hay **Modo** (por ahora vacio), **Comer** (medialuna, muffin o dona), **Tomar** (te, licuado o mate), **Hora**, **Sensor** y **Salir**. Le dibujamos todas las comidas nuevas a color.

**Que aprendimos:**
- Para "alejar" la cara, **todas las medidas se multiplican por un numero**: con 1 la cara es normal y con 0,5 queda a la mitad. Asi el mismo dibujo sirve de cerca y de lejos.
- Si apretas el boton A mientras Flufi esta en medio de un juego, el juego se **corta** enseguida: cada cuadro de la animacion pregunta "¿apretaron el boton?" antes de seguir.

---

## 2026-11-05 — Rulos rubios y labios de diva 💋

**Que hicimos:** Como la pantalla de Flufi es mas grande que la de Pitri, le pusimos **pelo**: rulos rubios arriba de la cara, un flequillo de rulitos y unos bucles que caen a los costados. Para que entren, bajamos un poquito la cara. Tambien le hicimos una boca mas ancha, con **labios carnosos pintados de rojo**, como una diva, ¡con brillito y todo! Cuando esta enamorada tira un besito con forma de corazon, y lo que dice mientras come ahora aparece en un **globito** de historieta.

**Que aprendimos:**
- Cada rulo es un **circulo** dorado con una vueltita mas oscura adentro. Poniendo muchos circulos pisandose un poquito, parece pelo enrulado.
- Los labios se dibujan **columna por columna**: para cada lugar de izquierda a derecha, una cuenta dice donde empieza el labio de arriba y donde termina el de abajo. El labio de arriba tiene un huequito en el medio: se llama **arco de cupido**.
- El pelo se mueve **menos** que los ojos cuando Flufi mira para un costado. Asi parece que gira la cabeza.
- Al principio Flufi se trababa: el **perro guardian** (watchdog) avisaba que el sistema no tenia tiempo de descansar. Pasaba porque su chip no tiene calculadora de decimales, y calcular la vueltita de 22 rulos en cada cuadro le costaba muchisimo. Lo arreglamos calculando las vueltitas **una sola vez** al prenderse y guardandolas en una tablita.

---

## 2026-11-05 — La hora y el sensor, en punto ⏰🌡️

**Que hicimos:** Igual que Pitri, Flufi muestra la **hora en punto** cada 5 minutos (x:00, x:05, x:10...), y cada 15 minutos (x:00, x:15, x:30, x:45) muestra la **temperatura** y la **presion del aire** que mide su sensor BMP280. Lo escribe abajo de sus ojitos.

**Que aprendimos:**
- La **presion del aire** es cuanto "pesa" el aire que tenemos encima. Se mide en hectopascales (hPa): al nivel del mar es mas o menos 1013. Cuando baja, muchas veces va a llover.
- Cada sensor BMP280 viene de fabrica con unos **numeros de calibracion** propios. Con esos numeros se corrigen las mediciones para que den bien.

---

## 2026-11-05 — Emociones de colores 🌈

**Que hicimos:** Flufi aprendio las mismas emociones que Pitri (contenta, sorpresa, enojada, triste, mareada, enamorada, verano, sospecha, sueño...), pero con una diferencia: **cada emocion prende el LED de la placa de un color**. Contenta es naranja, enamorada es rojo, triste es verde, y cuando se duerme el LED se apaga. Cuando saluda, el LED hace **arco iris**.

Sus lentes de sol son **"ojo de gato"**: marco rosa con las puntas para arriba, vidrio color ciruela y una piedrita brillante en cada punta.

Tambien le dejamos listos los dibujos de la comida y la bebida, ¡a color!, para cuando tenga su menu.

**Que aprendimos:**
- El LED mezcla tres lucecitas: **roja, verde y azul**. Con distintas cantidades de cada una sale cualquier color, igual que en la pantalla.
- El arco iris es dar la vuelta a la **rueda de colores**: rojo, amarillo, verde, celeste, azul, violeta y otra vez rojo.
- Ese LED es muy fuerte: lo usamos al 25 % de brillo para que no encandile.

---

## 2026-11-05 — WiFi, hora y saludo 📶👋

**Que hicimos:**
- Al prenderse, Flufi dice **"¡Hola Sofi!"** y **"Soy Flufi"**.
- Mientras se conecta al WiFi, abajo de sus ojitos aparecen unas **ondas** que se encienden de a una. Cuando se conecta, dice **"Tengo wifi!"**.
- Le pregunta la hora a internet y **saluda segun el momento del dia**: "¡Buen dia!", "¡Buenas tardes!" o "¡Buenas noches!".
- Apagamos el LED de colores de la placa.

**Que aprendimos:**
- El LED de la placa es "inteligente" (un WS2812): no se apaga cortandole la corriente, hay que mandarle un mensaje con el color "apagado".
- **Los secretos no se publican:** la contraseña del WiFi vive en un archivo que se queda en la compu y nunca se sube al repo.
- La pantalla de Flufi es cuadrada, asi que abajo de los ojos sobra lugar para escribir.

---

## 2026-11-05 — ¡Nace Flufi! 🐣

**Que hicimos:** Reciclamos la placa del proyecto color1.44 (un ESP32-C3 con una pantalla a
color) y le dibujamos a Flufi sus primeros ojitos: celestes, con **pestañas rosas**, que
parpadean cada tanto.

**Que aprendimos:**
- La pantalla de Flufi es **a color**: cada puntito puede tener cualquier color, mezclando
  rojo, verde y azul.
- Las pestañas siguen al parpado cuando parpadea, y cuando el ojo queda cerrado apuntan
  para abajo, como un ojito dormido de dibujo animado.
