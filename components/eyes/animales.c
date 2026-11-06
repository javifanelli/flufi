// ─────────────────────────────────────────────────────────────────────────────
//  Los animales del modo Animales
// ─────────────────────────────────────────────────────────────────────────────
//
//  Flufi hace de animal, pero SIEMPRE con sus ojos, sus pestañas y sus labios
//  rojos. Cada animal le agrega sus partes: orejas, cuerno, alas, bigotes...
//
//  Cada animal tiene:
//    - dibujar_..._atras:    lo que va detras de los ojos (orejas, alas, crin)
//    - dibujar_..._adelante: lo que va delante (naricita, bigotes, cosas que trae)
//    - actuar_...:           lo que hace (unos 5 segundos)
//
//  Las medidas estan pensadas desde "mx" (el medio de la cara, de izquierda a
//  derecha) y "cy" (la altura del centro de los ojos).
//
#include "eyes_interno.h"

#include "esp_log.h"

static const char *TAG = "ANIMALES";

eyes_animal_t animal_actual = ANIMAL_NINGUNO;

// Cosas que aparecen solo mientras actua
static bool hay_estrellas = false;   // unicornio
static bool hay_queso     = false;   // ratoncita
static bool hay_cola      = false;   // zorrita

// ─── Herramientas ────────────────────────────────────────────────────────────

static void rect(float x, float y, float w, float h, color_t c)
{
    display_fill_rect((int)x, (int)y, (int)(w < 1 ? 1 : w), (int)(h < 1 ? 1 : h), c);
}

static void circulo(float cx, float cy, float r, color_t c)
{
    display_fill_circle((int)cx, (int)cy, (int)(r + 0.5f), c);
}

static void triangulo(float x0, float y0, float x1, float y1, float x2, float y2, color_t c)
{
    display_fill_triangle((int)x0, (int)y0, (int)x1, (int)y1, (int)x2, (int)y2, c);
}

static void linea(float x0, float y0, float x1, float y1, int grosor, color_t c)
{
    display_draw_line((int)x0, (int)y0, (int)x1, (int)y1, grosor, c);
}

// Un ovalo relleno, renglon por renglon
static void elipse(float cx, float cy, float rx, float ry, color_t c)
{
    if (rx < 1 || ry < 1) return;
    for (int dy = -(int)ry; dy <= (int)ry; dy++) {
        float medio = rx * sqrtf(1 - (dy * dy) / (ry * ry));
        rect(cx - medio, cy + dy, 2 * medio + 1, 1, c);
    }
}

// Estrellita de 4 puntas
static void estrella(float x, float y, float r, color_t c)
{
    rect(x - r, y, 2 * r + 1, 1, c);
    rect(x, y - r, 1, 2 * r + 1, c);
    rect(x - 1, y - 1, 3, 3, c);
}

// Va de -1 a 1 y vuelve, al ritmo de cara.fase ("veces" vueltas por cada +1)
static float onda(float veces)
{
    return sinf(cara.fase * veces * 2 * PI);
}

// Bigotes: tres de cada lado, abajo de los ojos ("mueve" los hace temblar)
static void bigotes(float mx, float y, float mueve, color_t c)
{
    for (int lado = -1; lado <= 1; lado += 2) {
        linea(mx + lado * 14, y,     mx + lado * 33, y - 4 + mueve, 1, c);
        linea(mx + lado * 14, y + 2, mx + lado * 34, y + 2,         1, c);
        linea(mx + lado * 14, y + 4, mx + lado * 33, y + 8 - mueve, 1, c);
    }
}

// ═════════════════════════════════════════════════════════════════════════════
//  🐰 Coneja: orejas largas (una se dobla), naricita de corazon y dientitos
// ═════════════════════════════════════════════════════════════════════════════

#define CONEJA_BLANCO COLOR(245, 245, 245)
#define CONEJA_ROSA   COLOR(255, 160, 200)

// Una oreja larga: dos tramos rectos (el de arriba se puede doblar), hechos con
// circulitos uno al lado del otro. Los angulos se miden desde "para arriba".
static void oreja_larga(float x, float y, float ang1, float ang2, int lado)
{
    const float largo1 = 22, largo2 = 20;
    float codo_x = x + sinf(ang1) * largo1, codo_y = y - cosf(ang1) * largo1;
    for (int capa = 0; capa < 2; capa++) {              // primero lo blanco, despues lo rosa
        float r = capa ? 3 : 7;
        color_t c = capa ? CONEJA_ROSA : CONEJA_BLANCO;
        for (float t = 0; t <= largo1; t += 2)
            circulo(x + sinf(ang1) * t, y - cosf(ang1) * t, r, c);
        for (float t = 0; t <= largo2 - (capa ? 5 : 0); t += 2)
            circulo(codo_x + sinf(ang2) * t, codo_y - cosf(ang2) * t, r, c);
    }
}

static void dibujar_coneja_atras(float mx, float cy)
{
    float doblez = 0.2f + 1.0f * fmaxf(0, onda(1));     // la oreja derecha se dobla
    oreja_larga(mx - 15, cy - 22, -0.15f, -0.25f, -1);
    oreja_larga(mx + 15, cy - 22, 0.15f, 0.15f + doblez, 1);
}

static void dibujar_coneja_adelante(float mx, float cy)
{
    float mueve = onda(4);                              // la nariz se mueve
    dibujar_corazon(mx, cy + 8 + mueve * 0.7f, 2, CONEJA_ROSA);
    bigotes(mx, cy + 20, mueve, COLOR(200, 200, 200));

    if (cara.feliz < 0.3f && cara.asombro < 0.3f) {     // dientitos sobre el labio de abajo
        float base = cy + cara.alto / 2 + 7;
        rect(mx - 3, base + 1, 3, 4, BLANCO);
        rect(mx + 1, base + 1, 3, 4, BLANCO);
    }
}

static void actuar_coneja(void)
{
    cara_t d = cara;
    float y_normal = d.y;

    for (int i = 0; i < 8; i++) {                       // olfatea y mueve la oreja
        d.fase += 0.1f;
        animar(d, 100);
    }
    for (int i = 0; i < 3; i++) {                       // saltitos
        globo = "Hop!";
        d.y = y_normal - 8; d.fase += 0.25f; animar(d, 160);
        d.y = y_normal;     d.fase += 0.25f; animar(d, 160);
        globo = NULL;
        esperar(80);
    }
    d.feliz = 1;
    animar(d, 200);
    esperar(700);
    d.feliz = 0;
    animar(d, 200);
}

// ═════════════════════════════════════════════════════════════════════════════
//  🦄 Unicornio: crin de arco iris (en vez de los rulos) y cuerno dorado
// ═════════════════════════════════════════════════════════════════════════════

static const color_t colores_arcoiris[6] = {
    COLOR(255, 60, 60), COLOR(255, 150, 0), COLOR(255, 230, 0),
    COLOR(60, 220, 90), COLOR(60, 150, 255), COLOR(180, 90, 255),
};

static void dibujar_unicornio_atras(float mx, float cy)
{
    // La crin: los rulos de Flufi, pero de colores (que se van corriendo)
    dibujar_rulos_de_colores(colores_arcoiris, 6, (int)(cara.fase * 6), BLANCO);

    // El cuerno, con sus rayitas en espiral
    const color_t oro = COLOR(255, 210, 60), oro_oscuro = COLOR(205, 145, 20);
    float base = cy - 21, punta = cy - 54;
    triangulo(mx - 6, base, mx + 6, base, mx, punta, oro);
    for (int k = 0; k < 4; k++) {
        float y = base - 4 - k * 7;
        float medio = 6 * (y - punta) / (base - punta);
        linea(mx - medio, y + 2, mx + medio, y - 2, 1, oro_oscuro);
    }
    rect(mx - 2, base - 12, 1, 6, COLOR(255, 245, 190));        // brillito
}

static void dibujar_unicornio_adelante(float mx, float cy)
{
    if (!hay_estrellas) return;
    static const color_t colores[4] = {
        COLOR(255, 240, 120), BLANCO, COLOR(255, 160, 220), COLOR(160, 220, 255),
    };
    // Estrellitas que giran alrededor de la cara y titilan
    for (int k = 0; k < 4; k++) {
        float a = cara.fase * PI + k * PI / 2;
        float r = 2 + 2 * fabsf(sinf(cara.fase * 4 * PI + k));
        estrella(mx + 50 * cosf(a), cy + 36 * sinf(a), r, colores[k]);
    }
}

static void actuar_unicornio(void)
{
    cara_t d = cara;
    float y_normal = d.y;

    arcoiris = true;                                    // el LED tambien hace arco iris
    hay_estrellas = true;
    d.feliz = 0.8f;
    for (int i = 0; i < 24; i++) {
        d.fase += 0.05f;
        d.y = y_normal + 3 * sinf(i * 0.6f);            // flota
        globo = (i >= 6 && i < 18) ? "¡Magia!" : NULL;
        animar(d, 90);
    }
    globo = NULL;
    hay_estrellas = false;
    d.y = y_normal;
    d.feliz = 0;
    animar(d, 200);
    arcoiris = false;
}

// ═════════════════════════════════════════════════════════════════════════════
//  🐭 Ratoncita: orejas redondas grandes, naricita, bigotes y un quesito
// ═════════════════════════════════════════════════════════════════════════════

static void dibujar_ratoncita_atras(float mx, float cy)
{
    const color_t gris = COLOR(170, 170, 185), rosa = COLOR(255, 160, 200);
    float tic = onda(2);                                // las orejas se mueven al olfatear
    for (int lado = -1; lado <= 1; lado += 2) {
        circulo(mx + lado * 36, cy - 36 + tic, 16, gris);
        circulo(mx + lado * 36, cy - 35 + tic, 10, rosa);
    }
}

static void dibujar_queso(float x, float y)
{
    const color_t queso = COLOR(255, 210, 50), agujero = COLOR(215, 165, 25);
    rect(x - 14, y, 28, 9, queso);
    triangulo(x - 14, y, x + 14, y, x + 14, y - 9, queso);
    circulo(x - 6, y + 4, 2, agujero);
    circulo(x + 5, y + 3, 2, agujero);
    circulo(x + 9, y - 2, 1.5f, agujero);
}

static void dibujar_ratoncita_adelante(float mx, float cy)
{
    float mueve = 2 * onda(3);                          // los bigotes tiemblan
    circulo(mx, cy + 9, 3, COLOR(255, 140, 180));       // naricita
    rect(mx - 1, cy + 7, 1, 1, BLANCO);
    bigotes(mx, cy + 20, mueve, COLOR(210, 210, 220));

    if (hay_queso) dibujar_queso(cara.comida_x, 104);
}

static void actuar_ratoncita(void)
{
    cara_t d = cara;

    for (int i = 0; i < 10; i++) {                      // olfatea
        d.fase += 0.08f;
        animar(d, 80);
    }
    hay_queso = true;                                   // ¡aparece un quesito!
    cara.comida_x = d.comida_x = 160;
    d.comida_x = 102;
    d.x = 6;
    animar(d, 500);
    globo = "¡Queso!";
    d.feliz = 1;
    animar(d, 200);
    esperar(1000);
    globo = NULL;
    d.feliz = 0;
    d.comida_x = 160;
    animar(d, 400);
    hay_queso = false;
    d.x = 0;
    animar(d, 200);
}

// ═════════════════════════════════════════════════════════════════════════════
//  🦋 Mariposa: alas de colores a los costados y antenitas
// ═════════════════════════════════════════════════════════════════════════════

static void dibujar_mariposa_atras(float mx, float cy)
{
    const color_t violeta = COLOR(170, 80, 255), rosa = COLOR(255, 120, 190), amarillo = COLOR(255, 230, 80);
    float abiertas = 0.35f + 0.65f * fabsf(cosf(cara.fase * 2 * PI));   // aletea

    for (int lado = -1; lado <= 1; lado += 2) {
        float rx = 13 * abiertas, x = mx + lado * (36 + rx);             // ala de arriba
        elipse(x, cy - 8, rx, 17, violeta);
        circulo(x, cy - 8, fminf(5, rx * 0.45f), rosa);
        circulo(x, cy - 8, fminf(1.5f, rx * 0.15f), amarillo);

        rx = 9 * abiertas; x = mx + lado * (34 + rx);                     // ala de abajo
        elipse(x, cy + 18, rx, 11, rosa);
        circulo(x, cy + 19, fminf(3, rx * 0.35f), violeta);
    }
}

static void dibujar_mariposa_adelante(float mx, float cy)
{
    const color_t antena = COLOR(130, 60, 210), bolita = COLOR(255, 120, 190);
    for (int lado = -1; lado <= 1; lado += 2) {
        linea(mx + lado * 5,  cy - 17, mx + lado * 10, cy - 34, 2, antena);
        linea(mx + lado * 10, cy - 34, mx + lado * 18, cy - 46, 2, antena);
        circulo(mx + lado * 18, cy - 47, 3, bolita);
    }
}

static void actuar_mariposa(void)
{
    cara_t d = cara;
    float y_normal = d.y;

    for (int i = 0; i < 26; i++) {                      // vuela aleteando
        d.fase += 0.12f;
        d.x = 10 * sinf(i * 0.18f);
        d.y = y_normal + 6 * sinf(i * 0.35f);
        globo = (i >= 4 && i < 16) ? "¡Fiu!" : NULL;
        animar(d, 70);
    }
    globo = NULL;
    d.fase = ceilf(d.fase);                             // termina con las alas abiertas
    d.x = 0;
    d.y = y_normal;
    d.feliz = 0.5f;
    animar(d, 250);
    cerrar_y_abrir(false, true, 300);                   // guiño
    d.feliz = 0;
    animar(d, 200);
}

// ═════════════════════════════════════════════════════════════════════════════
//  🦊 Zorrita: orejas en punta, cachetes blancos, naricita y una cola esponjosa
// ═════════════════════════════════════════════════════════════════════════════

#define ZORRO_NARANJA COLOR(255, 130, 30)
#define ZORRO_CREMA   COLOR(255, 225, 190)
#define ZORRO_OSCURO  COLOR(90, 40, 20)

static void dibujar_zorrita_atras(float mx, float cy)
{
    // Pelito naranja arriba de los ojos, con un mechon blanco en el medio
    display_fill_round_rect((int)(mx - 44), (int)(cy - 34), 88, 16, 8, ZORRO_NARANJA);
    triangulo(mx - 6, cy - 18, mx + 6, cy - 18, mx, cy - 30, BLANCO);

    for (int lado = -1; lado <= 1; lado += 2) {
        float tic = (lado > 0) ? 3 * fmaxf(0, onda(2)) : 0;            // la oreja derecha se mueve
        float px = mx + lado * (34 + tic), py = cy - 62 + tic;          // la punta
        triangulo(mx + lado * 10, cy - 20, mx + lado * 44, cy - 20, px, py, ZORRO_NARANJA);
        triangulo(mx + lado * 17, cy - 21, mx + lado * 38, cy - 21, mx + lado * (32 + tic), cy - 50 + tic, ZORRO_CREMA);
        triangulo(mx + lado * (31 + tic), cy - 55 + tic, mx + lado * (37 + tic), cy - 55 + tic, px, py, ZORRO_OSCURO);

        circulo(mx + lado * 40, cy + 14, 9, BLANCO);                    // cachetes
        circulo(mx + lado * 47, cy + 6, 6, BLANCO);
    }
}

static void dibujar_cola(void)
{
    // Una cola esponjosa que sale de abajo a la derecha y se mueve de lado a lado
    float ang = -2.3f + 0.45f * onda(1);
    const float x0 = LCD_H_RES + 2, y0 = LCD_V_RES + 2, largo = 40;
    for (float t = 0; t <= 1; t += 0.06f) {
        float r = 4 + 9 * sinf(PI * (0.15f + 0.85f * t));
        circulo(x0 + cosf(ang) * largo * t, y0 + sinf(ang) * largo * t, r, (t > 0.75f) ? BLANCO : ZORRO_NARANJA);
    }
}

static void dibujar_zorrita_adelante(float mx, float cy)
{
    display_fill_round_rect((int)(mx - 3), (int)(cy + 7), 7, 5, 2, ZORRO_OSCURO);   // naricita
    rect(mx - 1, cy + 8, 1, 1, BLANCO);
    if (hay_cola) dibujar_cola();
}

static void actuar_zorrita(void)
{
    cara_t d = cara;

    for (int i = 0; i < 6; i++) {                       // mueve la oreja
        d.fase += 0.1f;
        animar(d, 90);
    }
    d.abre_izq = d.abre_der = 0.6f;                     // mirada picara, de un lado al otro
    d.feliz = 0.4f;
    d.x = -12; animar(d, 400);
    esperar(250);
    d.x = 12;  animar(d, 500);
    esperar(250);
    d.x = 0;   animar(d, 300);

    hay_cola = true;                                    // mueve la cola
    globo = "Je je!";
    for (int i = 0; i < 14; i++) {
        d.fase += 0.08f;
        animar(d, 80);
    }
    globo = NULL;
    hay_cola = false;
    d.abre_izq = d.abre_der = 1;
    d.feliz = 0;
    animar(d, 250);
}

// ═════════════════════════════════════════════════════════════════════════════
//  La lista de animales
// ═════════════════════════════════════════════════════════════════════════════

typedef struct {
    const char *nombre;
    float ancho, alto, y;                   // tamaño y altura de los ojos
    color_led_t led;                        // color del LED mientras es ese animal
    void (*atras)(float mx, float cy);
    void (*adelante)(float mx, float cy);
    void (*actuar)(void);
} animal_info_t;

static const animal_info_t animales[CANT_ANIMALES] = {
    //                      nombre       ancho alto  y   LED
    [ANIMAL_CONEJA]    = { "coneja",    24,   30,   6, LED(255, 255, 255), dibujar_coneja_atras,    dibujar_coneja_adelante,    actuar_coneja    },
    [ANIMAL_UNICORNIO] = { "unicornio", 30,   40,   2, LED(200, 80, 255),  dibujar_unicornio_atras, dibujar_unicornio_adelante, actuar_unicornio },
    [ANIMAL_RATONCITA] = { "ratoncita", 24,   30,   6, LED(255, 200, 0),   dibujar_ratoncita_atras, dibujar_ratoncita_adelante, actuar_ratoncita },
    [ANIMAL_MARIPOSA]  = { "mariposa",  26,   32,   4, LED(170, 60, 255),  dibujar_mariposa_atras,  dibujar_mariposa_adelante,  actuar_mariposa  },
    [ANIMAL_ZORRITA]   = { "zorrita",   24,   30,   6, LED(255, 80, 0),    dibujar_zorrita_atras,   dibujar_zorrita_adelante,   actuar_zorrita   },
};

static bool es_valido(eyes_animal_t a)
{
    return a > ANIMAL_NINGUNO && a < CANT_ANIMALES;
}

// La cara de base de un animal (tamaño y altura de los ojos)
static cara_t cara_de(eyes_animal_t a)
{
    cara_t c = CARA_NORMAL;
    if (!es_valido(a)) return c;
    c.ancho = animales[a].ancho;
    c.alto  = animales[a].alto;
    c.y     = animales[a].y;
    return c;
}

// ─── Lo que usa eyes.c ───────────────────────────────────────────────────────

// Los animales no tienen rulos (el unicornio tiene su crin de arco iris)
bool animal_lleva_rulos(void)
{
    return !es_valido(animal_actual);
}

void dibujar_animal_atras(float mx, float cy)
{
    if (es_valido(animal_actual)) animales[animal_actual].atras(mx, cy);
}

void dibujar_animal_adelante(float mx, float cy)
{
    if (es_valido(animal_actual)) animales[animal_actual].adelante(mx, cy);
}

// ─── Lo que usa main.c ───────────────────────────────────────────────────────

// Cierra los ojos, cambia de disfraz y los vuelve a abrir
static void cambiar_a(eyes_animal_t nuevo)
{
    cara_t d = cara;
    d.abre_izq = d.abre_der = 0;
    animar(d, 150);

    animal_actual = nuevo;
    hay_estrellas = hay_queso = hay_cola = false;
    globo = NULL;
    cara = cara_de(nuevo);
    cara.abre_izq = cara.abre_der = 0;
    if (es_valido(nuevo)) poner_led(animales[nuevo].led);
    else                  poner_led_tranquila();
    animar(cara_de(nuevo), 200);
}

void eyes_actuar(eyes_animal_t animal)
{
    if (!es_valido(animal)) return;
    empezar_accion();
    if (animal != animal_actual) cambiar_a(animal);

    ESP_LOGI(TAG, "Actuando de %s", animales[animal].nombre);
    animales[animal].actuar();
    globo = NULL;
    hay_estrellas = hay_queso = hay_cola = false;
    arcoiris = false;
    poner_led(animales[animal].led);
}

void eyes_sacar_disfraz(void)
{
    if (animal_actual == ANIMAL_NINGUNO) return;
    cambiar_a(ANIMAL_NINGUNO);
}
