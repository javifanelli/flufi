#include "eyes_interno.h"
#include "led.h"

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_random.h"
#include "esp_log.h"

static const char *TAG = "OJOS";

// ─── Forma de los ojos (¡jueguen con estos numeros!) ─────────────────────────
#define OJO_ANCHO        30   // pixeles
#define OJO_ALTO         40
#define OJO_RADIO        11   // que tan redondeadas son las esquinas
#define OJO_SEPARACION   22   // espacio entre los dos ojos
#define OJO_ALTO_CERRADO 3    // grosor de la rayita cuando esta cerrado
#define OJOS_Y           66   // altura del centro de los ojos (la pantalla mide 128)
#define PESTANA_GROSOR   2

// Hasta donde se pueden mover los ojos
#define MAX_X  ((LCD_H_RES - 2 * OJO_ANCHO - OJO_SEPARACION) / 2)   // 23
#define MAX_Y  12

#define CUADRO_MS        30   // cada cuanto se dibuja un cuadro de la animacion

// Cara alejada (cuando muestra la hora, el sensor o dice algo): se achica y se
// corre a la izquierda, y el texto aparece a la derecha
#define LEJOS_TAMANO     0.5f // la cara queda a la mitad de su tamaño
#define LEJOS_X          (-32) // cuanto se corre a la izquierda
#define COSTADO_DESDE    68   // donde empieza la zona del texto, a la derecha
#define COSTADO_ANCHO    (LCD_H_RES - COSTADO_DESDE - 2)

// ─── Color del LED para cada emocion (¡jueguen con estos colores!) ───────────
// LED(rojo, verde, azul), de 0 a 255

#define LED_TRANQUILA  LED(255, 80, 180)    // rosa
#define LED_CONTENTA   LED(255, 110, 0)     // naranja
#define LED_GUINO      LED(150, 0, 255)     // violeta
#define LED_SORPRESA   LED(0, 180, 255)     // celeste
#define LED_ENOJADA    LED(120, 0, 20)      // bordo
#define LED_TRISTE     LED(0, 255, 0)       // verde
#define LED_MAREADA    LED(255, 255, 255)   // blanco
#define LED_ENAMORADA  LED(255, 0, 0)       // rojo
#define LED_VERANO     LED(255, 200, 0)     // amarillo
#define LED_SOSPECHA   LED(0, 0, 255)       // azul
#define LED_SUENO      LED(0, 0, 0)         // apagado

// ─── Colores de los dibujos ──────────────────────────────────────────────────
#define COLOR_CORAZON   COLOR(255, 40, 80)
#define COLOR_LAGRIMA   COLOR(190, 230, 255)
#define COLOR_MARCO     COLOR(255, 120, 190)   // marco de los lentes (rosa)
#define COLOR_VIDRIO    COLOR(80, 25, 60)      // vidrio color ciruela
#define COLOR_BRILLO    COLOR(255, 190, 225)   // brillitos del vidrio
#define COLOR_PELO      COLOR(255, 205, 70)    // rulos rubios
#define COLOR_PELO_SOMBRA COLOR(200, 135, 30)  // la vueltita de cada rulo
#define COLOR_PELO_BRILLO COLOR(255, 245, 180) // el brillo del pelo
#define COLOR_LABIOS_BRILLO COLOR(255, 170, 180) // el brillito del lapiz labial
#define COLOR_GLOBO     BLANCO                 // globito de lo que dice
#define COLOR_GLOBO_TEXTO COLOR(150, 20, 70)

const cara_t CARA_NORMAL = {
    .ancho = OJO_ANCHO, .alto = OJO_ALTO, .abre_izq = 1, .abre_der = 1,
};

cara_t cara;
color_t color_fondo, color_fondo_normal;
static color_t color_ojos, color_pestanas, color_boca, color_texto;

static char texto_costado[64];        // texto al costado de la cara (eyes_decir)
static int  texto_escala = 2;

// Tamaño de la cara en este cuadro: 1 = normal, mas chico = mas lejos.
// Todas las medidas de la cara se multiplican por este numero.
static float z = 1;
const char *globo = NULL;      // texto de arriba (lo que dice mientras come, etc.)

static comida_t comida_actual = COMIDA_NINGUNA;
static int mordidas = 0;

// ─── El LED ──────────────────────────────────────────────────────────────────

static color_led_t led_deseado;
static color_led_t led_puesto = { 1, 2, 3 };   // distinto de todo, para que se mande la primera vez
bool arcoiris = false;

void poner_led(color_led_t c) { led_deseado = c; }

static void actualizar_led(void)
{
    if (arcoiris) {
        led_arcoiris_paso();
        led_puesto.r = 1;                     // para que al terminar se vuelva a mandar el color
        return;
    }
    if (led_deseado.r != led_puesto.r || led_deseado.g != led_puesto.g || led_deseado.b != led_puesto.b) {
        led_color(led_deseado.r, led_deseado.g, led_deseado.b);
        led_puesto = led_deseado;
    }
}

// ─── Ayudantes ───────────────────────────────────────────────────────────────

int azar(int min, int max)
{
    return min + (int)(esp_random() % (uint32_t)(max - min + 1));
}

/*
 * main.c le puede dar a los ojos una funcion que diga "¡para!" (por ejemplo,
 * cuando se aprieta el boton del menu). Cada cuadro de animacion y cada espera
 * la consultan: si dice que si, lo que queda del juego se saltea y la funcion
 * que se estaba haciendo termina enseguida.
 */
static bool (*hay_que_cortar)(void) = NULL;
static bool cortado = false;

static bool cortar_ahora(void)
{
    if (!cortado && hay_que_cortar != NULL && hay_que_cortar()) cortado = true;
    return cortado;
}

// Las funciones publicas la llaman al empezar (lo de antes ya no cuenta como cortado)
void empezar_accion(void)
{
    cortado = false;
}

void poner_led_tranquila(void)
{
    poner_led(LED_TRANQUILA);
}

void esperar(int ms)
{
    for (int t = 0; t < ms && !cortar_ahora(); t += 20)
        vTaskDelay(pdMS_TO_TICKS(ms - t < 20 ? ms - t : 20));
}

void arco(float cx, float cy, float r, float desde, float hasta, int grosor, color_t c)
{
    for (float a = desde; a <= hasta; a += 0.04f)
        display_fill_rect((int)(cx + r * cosf(a)) - grosor / 2, (int)(cy + r * sinf(a)) - grosor / 2,
                          grosor, grosor, c);
}

static void anillo(int cx, int cy, int r_afuera, int r_adentro, color_t c)
{
    display_fill_circle(cx, cy, r_afuera, c);
    display_fill_circle(cx, cy, r_adentro, color_fondo);
}

// ─── Dibujos de cada parte ───────────────────────────────────────────────────

/*
 * Las pestañas: tres rayitas que salen del borde de arriba del ojo, del lado
 * de afuera, abiertas como un abanico (lado = -1 ojo izquierdo, +1 derecho).
 * Si el ojo esta casi cerrado, apuntan para abajo (ojito cerrado).
 */
static const struct { int base_x, base_y, punta_x, punta_y; } pestanas[3] = {
    { 13, 3, 21, -5 },
    {  8, 1, 13, -8 },
    {  2, 0,  4, -9 },
};

static void dibujar_pestanas(float cx, float arriba, int lado, float abre)
{
    int hacia = (abre < 0.3f) ? -1 : 1;
    int grosor = (z < 0.75f) ? 1 : PESTANA_GROSOR;
    for (int k = 0; k < 3; k++) {
        display_draw_line((int)(cx + lado * pestanas[k].base_x * z), (int)(arriba + pestanas[k].base_y * hacia * z),
                          (int)(cx + lado * pestanas[k].punta_x * z), (int)(arriba + pestanas[k].punta_y * hacia * z),
                          grosor, color_pestanas);
    }
}

static void dibujar_espiral(float cx, float cy, float radio, float giro)
{
    const float fin = 2.5f * 2 * PI;
    for (float a = 0; a < fin; a += 0.05f) {
        float r = radio * a / fin;
        display_fill_rect((int)(cx + r * cosf(a + giro)), (int)(cy + r * sinf(a + giro)), 2, 2, color_ojos);
    }
}

void dibujar_corazon(float cx, float cy, float r, color_t c)
{
    if (r < 1) r = 1;
    int y0 = (int)(cy - r * 0.5f);
    display_fill_circle((int)(cx - r), y0, (int)(r + 0.5f), c);
    display_fill_circle((int)(cx + r), y0, (int)(r + 0.5f), c);
    display_fill_triangle((int)(cx - 2 * r), y0, (int)(cx + 2 * r), y0, (int)cx, (int)(y0 + 2 * r), c);
}

static void dibujar_lagrima(int x, int y)
{
    display_fill_circle(x, y, 3, COLOR_LAGRIMA);
    display_fill_triangle(x - 3, y, x + 3, y, x, y - 7, COLOR_LAGRIMA);
}

static void dibujar_z(int x, int y, int s)
{
    display_fill_rect(x, y, s, 2, color_texto);
    display_fill_rect(x, y + s - 2, s, 2, color_texto);
    for (int k = 0; k < s; k++) display_fill_rect(x + s - 1 - k, y + k, 2, 1, color_texto);
}

// Lentes de sol "ojo de gato": marco rosa con las puntas de afuera para arriba,
// vidrio color ciruela con brillitos y una piedrita brillante en cada punta.
// "puestos" va de 0 (arriba, afuera de la pantalla) a 1 (en los ojos).
static void dibujar_lentes(float cx_izq, float cx_der, float cy, float puestos)
{
    const int w = OJO_ANCHO + 10, h = 28;
    int arriba = (int)(cy - 2 - h / 2 - (1 - puestos) * 80);
    int xi = (int)(cx_izq - w / 2.0f), xd = (int)(cx_der - w / 2.0f);

    display_fill_rect(0, arriba + 4, xi, 2, COLOR_MARCO);                       // patillas
    display_fill_rect(xd + w, arriba + 4, LCD_H_RES - (xd + w), 2, COLOR_MARCO);
    display_fill_rect(xi + w, arriba + 3, xd - (xi + w), 2, COLOR_MARCO);       // puente finito

    for (int lado = -1; lado <= 1; lado += 2) {
        int x = (lado < 0) ? xi : xd;
        int afuera = (lado < 0) ? x : x + w;              // borde de afuera del lente

        // Punta de "ojo de gato": un triangulo que sube hacia afuera
        display_fill_triangle(afuera - lado * 16, arriba, afuera - lado * 2, arriba + 14,
                              afuera + lado * 7, arriba - 7, COLOR_MARCO);

        display_fill_round_rect(x, arriba, w, h, 10, COLOR_MARCO);                // marco
        display_fill_round_rect(x + 2, arriba + 2, w - 4, h - 4, 8, COLOR_VIDRIO); // vidrio
        for (int k = 0; k < 11; k++)                                              // brillitos
            display_fill_rect(x + 8 + k, arriba + 17 - k, 3, 1, COLOR_BRILLO);
        for (int k = 0; k < 5; k++)
            display_fill_rect(x + 18 + k, arriba + 19 - k, 1, 1, COLOR_BRILLO);

        display_fill_circle(afuera + lado * 5, arriba - 5, 2, BLANCO);           // piedrita
        display_set_pixel(afuera + lado * 5, arriba - 9, BLANCO);                 // con su destello
        display_set_pixel(afuera + lado * 9, arriba - 5, BLANCO);
    }
}

/*
 * El pelo: rulos rubios arriba de la cara. Cada rulo es un circulo dorado con
 * una vueltita mas oscura adentro (para que se vea enrulado) y un brillito.
 * Hay una fila de rulos grandes, un flequillo de rulos mas chicos y unos
 * bucles que caen a los costados. {x desde el centro, y, radio}
 */
static const struct { int8_t x, y, r; } rulos[] = {
    // fila de arriba
    { -56, 4, 8 }, { -42, 3, 8 }, { -28, 2, 8 }, { -14, 1, 8 }, { 0, 1, 8 },
    {  14, 1, 8 }, {  28, 2, 8 }, {  42, 3, 8 }, {  56, 4, 8 },
    // flequillo
    { -48, 14, 6 }, { -36, 13, 6 }, { -24, 13, 6 }, { -12, 14, 6 }, { 0, 13, 6 },
    {  12, 14, 6 }, {  24, 13, 6 }, {  36, 13, 6 }, {  48, 14, 6 },
    // bucles de los costados
    { -59, 20, 6 }, { -61, 31, 5 }, {  59, 20, 6 }, {  61, 31, 5 },
};

/*
 * La vueltita de cada rulo se calcula UNA sola vez y se guarda en una tablita.
 * El ESP32-C3 no tiene "calculadora de decimales" (FPU): cada seno y coseno le
 * cuesta mucho, y hacerlo para 22 rulos en cada cuadro lo dejaba sin aire.
 */
#define RULO_RADIO_MAX 8
#define VUELTITA_MAX   40
static struct { int8_t dx[VUELTITA_MAX], dy[VUELTITA_MAX]; uint8_t n; } vueltitas[RULO_RADIO_MAX + 1];

static void preparar_vueltitas(void)
{
    for (int r = 1; r <= RULO_RADIO_MAX; r++) {
        vueltitas[r].n = 0;
        for (float a = 0.2f * PI; a <= 1.7f * PI; a += 0.04f) {
            int8_t dx = (int8_t)(r * 0.5f * cosf(a)), dy = (int8_t)(r * 0.5f * sinf(a));
            bool repetido = false;            // guardamos cada pixel una sola vez
            for (int k = 0; k < vueltitas[r].n; k++)
                if (vueltitas[r].dx[k] == dx && vueltitas[r].dy[k] == dy) { repetido = true; break; }
            if (!repetido && vueltitas[r].n < VUELTITA_MAX) {
                vueltitas[r].dx[vueltitas[r].n] = dx;
                vueltitas[r].dy[vueltitas[r].n] = dy;
                vueltitas[r].n++;
            }
        }
    }
}

static void dibujar_rulo(int x, int y, int r, color_t color, color_t vueltita)
{
    if (r > RULO_RADIO_MAX) r = RULO_RADIO_MAX;
    display_fill_circle(x, y, r, color);
    for (int k = 0; k < vueltitas[r].n; k++)                             // la vueltita
        display_set_pixel(x + vueltitas[r].dx[k], y + vueltitas[r].dy[k], vueltita);
    display_fill_rect(x - r / 2, y - r + 2, 2, 1, COLOR_PELO_BRILLO);    // brillito
}

void dibujar_rulos_de_colores(const color_t *colores, int cantidad, int corrimiento, color_t vueltita)
{
    // El pelo acompaña a la cara cuando mira para los costados (un poquito menos).
    // Con la cara alejada se mueve junto con ella y se achica.
    float hx = LCD_H_RES / 2.0f + cara.x * (0.5f + 0.5f * cara.lejos);
    float hy = cara.y * (0.3f + 0.7f * cara.lejos);
    const float medio = LCD_V_RES / 2.0f;     // la cara se achica hacia el centro
    for (unsigned k = 0; k < sizeof(rulos) / sizeof(rulos[0]); k++) {
        int r = (int)(rulos[k].r * z + 0.5f);
        if (r < 2) r = 2;
        // El color depende de la "columna" del rulo (de izquierda a derecha)
        int columna = (rulos[k].x + 64) / 15;
        color_t c = colores[((columna + corrimiento) % cantidad + cantidad) % cantidad];
        dibujar_rulo((int)(hx + rulos[k].x * z), (int)(medio + (rulos[k].y - medio) * z + hy), r, c, vueltita);
    }
}

static void dibujar_pelo(void)
{
    static const color_t rubio[1] = { COLOR_PELO };
    dibujar_rulos_de_colores(rubio, 1, 0, COLOR_PELO_SOMBRA);
}

// Globito de dialogo arriba a la derecha (lo que dice mientras come, etc.)
static void dibujar_globo(const char *texto)
{
    int w = display_text_width(texto, 2) + 10;
    int h = display_text_height(texto, 2) + 8;
    int x = LCD_H_RES - w - 2, y = 2;
    display_fill_round_rect(x, y, w, h, 6, COLOR_GLOBO);
    display_fill_triangle(x + 8, y + h - 1, x + 18, y + h - 1, x + 4, y + h + 7, COLOR_GLOBO);   // la colita
    display_draw_text(x + 5, y + 4, texto, 2, COLOR_GLOBO_TEXTO);
}

// lado: -1 = ojo izquierdo, +1 = ojo derecho
static void dibujar_ojo(float cx, float cy, float ancho, float alto, float abre, int lado)
{
    if (cara.espiral > 0.5f) {
        dibujar_espiral(cx, cy, fminf(ancho, alto) / 2 - 1, cara.giro);
        return;
    }
    if (cara.corazon > 0.5f) {
        dibujar_corazon(cx, cy, ancho / 4, COLOR_CORAZON);
        return;
    }

    float cerrado = fmaxf(2, OJO_ALTO_CERRADO * z);
    int h = (int)(cerrado + (alto - cerrado) * abre);
    int w = (int)ancho;
    int x = (int)(cx - w / 2.0f);
    int y = (int)(cy - h / 2.0f);
    int x_adentro = (lado < 0) ? x + w + 2 : x - 2;
    int x_afuera  = (lado < 0) ? x - 2     : x + w + 2;

    display_fill_round_rect(x, y, w, h, (int)(OJO_RADIO * z), color_ojos);

    // Ojitos felices: una "loma" del color del fondo tapa la parte de abajo
    if (cara.feliz > 0.01f && h > cerrado) {
        int tapa = (int)(h * 0.6f * cara.feliz);
        display_fill_round_rect(x - 4, y + h - tapa, w + 8, h, (w + 8) / 2, color_fondo);
    }
    // Enojada: corta la esquina de arriba que da a la nariz
    if (cara.enojo > 0.01f)
        display_fill_triangle(x_afuera, y - 2, x_adentro, y - 2, x_adentro, y + (int)(h * 0.55f * cara.enojo), color_fondo);
    // Triste: corta la esquina de arriba del lado de afuera
    if (cara.triste > 0.01f)
        display_fill_triangle(x_adentro, y - 2, x_afuera, y - 2, x_afuera, y + (int)(h * 0.5f * cara.triste), color_fondo);

    dibujar_pestanas(cx, y, lado, abre);

    if (cara.lagrima > 0.01f && cara.lagrima < 0.99f) {
        int lx = (int)(cx + lado * w * 0.25f);
        int desde = y + h + 6, hasta = LCD_V_RES + 8;
        dibujar_lagrima(lx, desde + (int)((hasta - desde) * cara.lagrima));
    }
}

// ─── La boca ─────────────────────────────────────────────────────────────────
// Cada emocion tiene su boca. Gana la emocion mas fuerte; si ninguna llega a
// 0,3, Flufi esta tranquila y lleva una sonrisita sencilla.

typedef enum {
    BOCA_SENCILLA, BOCA_SONRISA, BOCA_TRISTE, BOCA_ENOJADA, BOCA_O, BOCA_MAREADA,
    BOCA_BESO, BOCA_CANCHERA, BOCA_SOSPECHA, BOCA_DORMIDA,
} boca_t;

/*
 * Labios de diva, pintados de rojo: el de arriba con el "arco de cupido" (dos
 * lomitas con un huequito en el medio) y el de abajo mas gordito. Se dibujan
 * columna por columna: para cada x se calcula donde empieza y donde termina.
 * "torcida" levanta un costado (sonrisa canchera).
 */
static void dibujar_labios(float mx, float base, float ancho, float torcida)
{
    int a = (int)(ancho * z + 0.5f);
    if (a < 3) a = 3;
    for (int dx = -a; dx <= a; dx++) {
        float u = (float)dx / a;                            // -1 a 1
        float d = dx / z;                                   // como si la cara fuera de tamaño normal
        float arriba = z * (5.0f * (1 - u * u) - 2.2f * expf(-(d * d) / 5.0f));   // arco de cupido
        float abajo  = z * 6.0f * sqrtf(1 - u * u);         // labio de abajo, carnoso
        float t = torcida * z * u;
        int y0 = (int)lroundf(base - arriba + t), y1 = (int)lroundf(base + abajo + t);
        display_fill_rect((int)mx + dx, y0, 1, y1 - y0 + 1, color_boca);
        if (dx > -a + 1 && dx < a - 1)                      // la rayita entre los labios
            display_set_pixel((int)mx + dx, (int)lroundf(base + t - 1.5f * z * u * u * u * u), color_fondo);
    }
    // Brillitos de "gloss"
    display_fill_rect((int)(mx + 3 * z), (int)lroundf(base + (3 + torcida * 0.25f) * z), z < 0.75f ? 1 : 3, 1,
                      COLOR_LABIOS_BRILLO);
    display_set_pixel((int)(mx - 6 * z), (int)lroundf(base - (3 + torcida * 0.5f) * z), COLOR_LABIOS_BRILLO);
}

// Sonrisa grande pintada de rojo: una "medialuna" que crece con la felicidad
static void dibujar_sonrisa(float mx, float cy, float feliz)
{
    int ancho = (int)((8 + 7 * feliz) * z);
    if (ancho < 3) ancho = 3;
    float esquinas = cy + (14 + (1 - feliz) * 8) * z;
    for (int dx = -ancho; dx <= ancho; dx++) {
        float u = (float)dx / ancho;
        float y0 = esquinas + 6 * z * feliz * (1 - u * u);
        float y1 = y0 + (2 + 5 * feliz * sqrtf(1 - u * u)) * z;
        display_fill_rect((int)mx + dx, (int)lroundf(y0), 1, (int)lroundf(y1 - y0) + 1, color_boca);
    }
    display_fill_rect((int)(mx + 3 * z), (int)lroundf(esquinas + 9 * feliz * z), z < 0.75f ? 1 : 3, 1,
                      COLOR_LABIOS_BRILLO);
}

static void dibujar_boca(float cy, float perspectiva)
{
    float mas_fuerte = 0.3f;
    boca_t boca = BOCA_SENCILLA;
#define CANDIDATA(peso, tipo) if ((peso) > mas_fuerte) { mas_fuerte = (peso); boca = (tipo); }
    CANDIDATA(cara.feliz,    BOCA_SONRISA);
    CANDIDATA(cara.triste,   BOCA_TRISTE);
    CANDIDATA(cara.enojo,    BOCA_ENOJADA);
    CANDIDATA(cara.asombro,  BOCA_O);
    CANDIDATA(cara.espiral,  BOCA_MAREADA);
    CANDIDATA(cara.corazon,  BOCA_BESO);
    CANDIDATA(cara.lentes,   BOCA_CANCHERA);
    CANDIDATA(cara.sospecha, BOCA_SOSPECHA);
    CANDIDATA(cara.sueno,    BOCA_DORMIDA);
#undef CANDIDATA

    float mx = LCD_H_RES / 2.0f + cara.x;

    if (boca == BOCA_SONRISA) {        // crece con la felicidad
        dibujar_sonrisa(mx, cy, cara.feliz);
        return;
    }

    // ¿Donde terminan los ojos? La boca va justo abajo
    float mitad_ojo;
    if (cara.espiral > 0.5f)      mitad_ojo = fminf(cara.ancho, cara.alto) / 2;
    else if (cara.corazon > 0.5f) mitad_ojo = 1.5f * cara.ancho / 4;
    else {
        float abre = (boca == BOCA_SENCILLA) ? 1 : fmaxf(cara.abre_izq, cara.abre_der);
        float alto = cara.alto * (1 + fabsf(perspectiva));
        mitad_ojo = (OJO_ALTO_CERRADO + (alto - OJO_ALTO_CERRADO) * abre) / 2;
    }
    if (cara.lentes > 0.5f) mitad_ojo = fmaxf(mitad_ojo, 14);
    mitad_ojo *= z;
    float y = cy + mitad_ojo + 4 * z;

    // Medidas de la cara normal, achicadas si la cara esta lejos
#define Z(n) ((n) * z)
    int g3 = (z < 0.75f) ? 2 : 3, g4 = (z < 0.75f) ? 2 : 4;

    switch (boca) {
    case BOCA_SENCILLA:                // labios de diva
        dibujar_labios(mx, y + Z(3), 12, 0);
        break;
    case BOCA_TRISTE:
        arco(mx, y + Z(13), Z(13), 1.2f * PI, 1.8f * PI, g4, color_boca);
        break;
    case BOCA_ENOJADA:
        for (int i = 0; i < 7; i++) {
            int x0 = (int)(mx + Z(-14 + i * 4));
            display_draw_line(x0, (int)(y + Z(i % 2 ? 5 : 1)), (int)(x0 + Z(4)), (int)(y + Z(i % 2 ? 1 : 5)), g3, color_boca);
        }
        break;
    case BOCA_O:                       // una "O" de labios gorditos
        anillo((int)mx, (int)(y + Z(6)), (int)Z(4 + 3 * cara.asombro), (int)Z(1 + 2 * cara.asombro), color_boca);
        break;
    case BOCA_MAREADA:
        for (int dx = -(int)Z(14); dx <= (int)Z(14); dx++)
            display_fill_rect((int)mx + dx, (int)(y + Z(2 + 2 * sinf(dx / z * 0.5f + cara.giro))), 2, g3, color_boca);
        break;
    case BOCA_BESO:                    // un besito en forma de corazon
        dibujar_corazon(mx, y + Z(6), Z(3.5f), color_boca);
        break;
    case BOCA_CANCHERA:                // labios de diva, con una sonrisa de costado
        dibujar_labios(mx + Z(2), y + Z(3), 12, -3);
        break;
    case BOCA_SOSPECHA:
        display_draw_line((int)(mx - Z(8)), (int)(y + Z(4)), (int)(mx + Z(8)), (int)(y + Z(1)), g3, color_boca);
        break;
    case BOCA_DORMIDA:
        anillo((int)mx, (int)(y + Z(4)), (int)Z(4), (int)Z(2), color_boca);
        break;
    default:
        break;
    }
#undef Z
}

// ─── Dibujar un cuadro ───────────────────────────────────────────────────────

// Ondas de WiFi al costado de la cara, que se van encendiendo de a una
static void dibujar_ondas(void)
{
    const float ox = COSTADO_DESDE + COSTADO_ANCHO / 2.0f, oy = 86;
    int ondas = ((int)(cara.fase * 4)) % 4;
    display_fill_circle((int)ox, (int)oy, 2, color_texto);
    for (int k = 0; k < ondas; k++)
        arco(ox, oy, 8 + 6 * k, 1.25f * PI, 1.75f * PI, 2, color_texto);
}

static void dibujar(void)
{
    // Lejos, la cara se achica (hacia el centro de la pantalla) y mira hacia el texto
    z = 1 - (1 - LEJOS_TAMANO) * cara.lejos;
    const float medio = LCD_V_RES / 2.0f;

    // Perspectiva: el ojo del lado hacia donde mira se ve un poquito mas grande
    float p = (cara.x / MAX_X) * 0.12f * (1 - cara.lejos) + 0.12f * cara.lejos;
    float cy     = medio + (OJOS_Y - medio) * z + cara.y;
    float cx_izq = LCD_H_RES / 2.0f + cara.x - (OJO_SEPARACION / 2.0f + cara.ancho / 2.0f) * z;
    float cx_der = LCD_H_RES / 2.0f + cara.x + (OJO_SEPARACION / 2.0f + cara.ancho / 2.0f) * z;

    float mx = LCD_H_RES / 2.0f + cara.x;

    display_clear(color_fondo);
    if (personaje_lleva_rulos()) dibujar_pelo();
    dibujar_personaje_atras(mx, cy);           // orejas, alas, sombrero... (si hace de personaje)
    dibujar_ojo(cx_izq, cy, cara.ancho * z, cara.alto * z * (1 - p), cara.abre_izq, -1);
    dibujar_ojo(cx_der, cy, cara.ancho * z, cara.alto * z * (1 + p), cara.abre_der, +1);
    dibujar_boca(cy, p);
    dibujar_personaje_adelante(mx, cy);        // naricita, cosas que trae...

    // Corazoncitos que suben por toda la pantalla
    if (cara.corazoncitos > 0.01f && cara.corazoncitos < 0.99f) {
        float t1 = cara.corazoncitos, t2 = fmodf(cara.corazoncitos + 0.5f, 1.0f);
        float recorrido = LCD_V_RES + 20;
        dibujar_corazon(10,              LCD_V_RES + 6 - recorrido * t1, 3, COLOR_CORAZON);
        dibujar_corazon(LCD_H_RES - 11,  LCD_V_RES + 6 - recorrido * t1, 3, COLOR_CORAZON);
        dibujar_corazon(LCD_H_RES / 2.0f, LCD_V_RES + 6 - recorrido * t2, 3, COLOR_CORAZON);
    }

    if (cara.lentes > 0.01f) dibujar_lentes(cx_izq, cx_der, cy, cara.lentes);

    if (cara.zzz > 0.01f && cara.zzz < 0.99f) {
        int s = 5 + (int)(7 * cara.zzz);
        dibujar_z((int)(cx_der + cara.ancho / 2 + 2 + 6 * cara.zzz), (int)(cy - 10 - 30 * cara.zzz), s);
    }

    if (comida_actual != COMIDA_NINGUNA)
        comida_dibujar(comida_actual, cara.comida_x, cara.comida_y, 0.9f, mordidas, color_fondo);

    if (cara.ondas > 0.5f) dibujar_ondas();
    if (cara.texto > 0.01f) {
        // El texto entra deslizandose desde la derecha
        int centro = COSTADO_DESDE + COSTADO_ANCHO / 2;
        int x = (int)(centro + (LCD_H_RES - COSTADO_DESDE) * (1 - cara.texto));
        display_draw_text_centered_at(texto_costado, x, -1, texto_escala, color_texto);
    }
    if (globo != NULL) dibujar_globo(globo);

    display_flush();
    actualizar_led();
}

// ─── Animar ──────────────────────────────────────────────────────────────────

static float mezclar(float a, float b, float t) { return a + (b - a) * t; }

// Pasa suavemente de la cara actual a "destino" en "ms" milisegundos
void animar(cara_t destino, int ms)
{
    if (cortar_ahora()) return;
    cara_t inicio = cara;
    int pasos = ms / CUADRO_MS;
    if (pasos < 1) pasos = 1;

    TickType_t ultimo = xTaskGetTickCount();
    for (int i = 1; i <= pasos; i++) {
        float t = (float)i / pasos;
        t = t * t * (3 - 2 * t);    // arranca y frena despacito

        cara.x            = mezclar(inicio.x,            destino.x,            t);
        cara.y            = mezclar(inicio.y,            destino.y,            t);
        cara.ancho        = mezclar(inicio.ancho,        destino.ancho,        t);
        cara.alto         = mezclar(inicio.alto,         destino.alto,         t);
        cara.abre_izq     = mezclar(inicio.abre_izq,     destino.abre_izq,     t);
        cara.abre_der     = mezclar(inicio.abre_der,     destino.abre_der,     t);
        cara.feliz        = mezclar(inicio.feliz,        destino.feliz,        t);
        cara.enojo        = mezclar(inicio.enojo,        destino.enojo,        t);
        cara.triste       = mezclar(inicio.triste,       destino.triste,       t);
        cara.espiral      = mezclar(inicio.espiral,      destino.espiral,      t);
        cara.giro         = mezclar(inicio.giro,         destino.giro,         t);
        cara.lagrima      = mezclar(inicio.lagrima,      destino.lagrima,      t);
        cara.zzz          = mezclar(inicio.zzz,          destino.zzz,          t);
        cara.corazon      = mezclar(inicio.corazon,      destino.corazon,      t);
        cara.corazoncitos = mezclar(inicio.corazoncitos, destino.corazoncitos, t);
        cara.lentes       = mezclar(inicio.lentes,       destino.lentes,       t);
        cara.asombro      = mezclar(inicio.asombro,      destino.asombro,      t);
        cara.sospecha     = mezclar(inicio.sospecha,     destino.sospecha,     t);
        cara.sueno        = mezclar(inicio.sueno,        destino.sueno,        t);
        cara.lejos        = mezclar(inicio.lejos,        destino.lejos,        t);
        cara.ondas        = mezclar(inicio.ondas,        destino.ondas,        t);
        cara.texto        = mezclar(inicio.texto,        destino.texto,        t);
        cara.fase         = mezclar(inicio.fase,         destino.fase,         t);
        cara.comida_x     = mezclar(inicio.comida_x,     destino.comida_x,     t);
        cara.comida_y     = mezclar(inicio.comida_y,     destino.comida_y,     t);

        dibujar();
        if (cortar_ahora()) return;
        // Espera hasta el proximo cuadro. Si dibujar tardo demasiado y ya estamos
        // atrasados, igual descansa un ratito: el resto del sistema necesita
        // turnos para trabajar (si no, salta el "perro guardian", el watchdog).
        if (xTaskDelayUntil(&ultimo, pdMS_TO_TICKS(CUADRO_MS)) == pdFALSE) {
            vTaskDelay(1);
            ultimo = xTaskGetTickCount();
        }
    }
}

void mover_a(float x, float y, int ms)
{
    cara_t d = cara;
    d.x = x;
    d.y = y;
    animar(d, ms);
}

void cerrar_y_abrir(bool izq, bool der, int cerrado_ms)
{
    cara_t abierta = cara, cerrada = cara;
    if (izq) cerrada.abre_izq = 0;
    if (der) cerrada.abre_der = 0;
    animar(cerrada, 70);
    esperar(cerrado_ms);
    animar(abierta, 110);
}

// ─── Juegos (cada uno con el color de LED de su emocion) ────────────────────

static void juego_mirar(void)
{
    poner_led(LED_TRANQUILA);
    mover_a(azar(-MAX_X, MAX_X), azar(-MAX_Y, MAX_Y), azar(150, 400));
    esperar(azar(400, 1500));
}

static void juego_centro(void)
{
    poner_led(LED_TRANQUILA);
    animar(CARA_NORMAL, 300);
    esperar(azar(300, 1000));
}

static void juego_doble_parpadeo(void)
{
    poner_led(LED_TRANQUILA);
    cerrar_y_abrir(true, true, 30);
    esperar(80);
    cerrar_y_abrir(true, true, 30);
}

static void juego_guino(void)
{
    poner_led(LED_GUINO);
    cara_t d = cara;
    d.feliz = 0.5f;
    animar(d, 150);
    bool izquierdo = azar(0, 1);
    cerrar_y_abrir(izquierdo, !izquierdo, 350);
    d = cara;
    d.feliz = 0;
    animar(d, 150);
}

static void juego_sorpresa(void)
{
    poner_led(LED_SORPRESA);
    cara_t d = cara;
    d.ancho = OJO_ANCHO + 6;
    d.alto  = OJO_ALTO + 8;
    d.y     = -6;
    d.asombro = 1;
    animar(d, 120);
    esperar(azar(500, 900));
    d.ancho = OJO_ANCHO;
    d.alto  = OJO_ALTO;
    d.y     = 0;
    d.asombro = 0;
    animar(d, 250);
}

static void juego_contento(void)
{
    poner_led(LED_CONTENTA);
    cara_t d = CARA_NORMAL;
    d.feliz = 1;
    animar(d, 200);
    for (int i = 0; i < 3; i++) {
        mover_a(0, -6, 120);
        mover_a(0, 0, 120);
    }
    esperar(400);
    animar(CARA_NORMAL, 200);
}

static void juego_girar(void)
{
    poner_led(LED_TRANQUILA);
    float r_x = MAX_X * 0.7f, r_y = MAX_Y * 0.8f;
    int sentido = azar(0, 1) ? 1 : -1;
    mover_a(r_x, 0, 150);
    for (int i = 1; i <= 16; i++) {
        float ang = sentido * i * 2 * PI / 16;
        mover_a(r_x * cosf(ang), r_y * sinf(ang), 60);
    }
    animar(CARA_NORMAL, 200);
}

static void juego_sospecha(void)
{
    poner_led(LED_SOSPECHA);
    cara_t d = CARA_NORMAL;
    d.abre_izq = d.abre_der = 0.35f;
    d.sospecha = 1;
    animar(d, 200);
    mover_a(-MAX_X, 0, 500);
    esperar(300);
    mover_a(MAX_X, 0, 700);
    esperar(300);
    animar(CARA_NORMAL, 250);
}

static void juego_sueno(void)
{
    poner_led(LED_SUENO);
    cara_t d = CARA_NORMAL;
    d.y = 4;
    d.abre_izq = d.abre_der = 0.3f;
    d.sueno = 1;
    animar(d, 1200);
    d.abre_izq = d.abre_der = 0;
    for (int i = 0; i < 2; i++) {            // duerme un ratito, con la "Z"
        cara.zzz = 0;
        d.zzz = 1;
        animar(d, 1200);
        d.zzz = 0;
    }
    cara.zzz = 0;
    poner_led(LED_TRANQUILA);
    animar(CARA_NORMAL, 100);                // ¡se desperto!
    juego_doble_parpadeo();
}

static void juego_pasear(void)
{
    poner_led(LED_TRANQUILA);
    int lado = azar(0, 1) ? 1 : -1;
    mover_a(lado * MAX_X, azar(-MAX_Y, MAX_Y), 400);
    esperar(300);
    mover_a(-lado * MAX_X, azar(-MAX_Y, MAX_Y), 700);
    esperar(300);
    mover_a(0, 0, 400);
}

static void juego_mareado(void)
{
    poner_led(LED_MAREADA);
    cara_t d = CARA_NORMAL;
    d.espiral = 1;
    animar(d, 60);
    for (int i = 0; i < 30; i++) {
        d.giro += 0.9f;
        d.x = 8 * cosf(i * 0.5f);
        d.y = 5 * sinf(i * 0.5f);
        animar(d, 60);
    }
    cara = CARA_NORMAL;                      // vuelve en si
    cara.abre_izq = cara.abre_der = 0;
    poner_led(LED_TRANQUILA);
    dibujar();
    for (int i = 0; i < 2; i++) {
        mover_a(-6, 0, 90);
        mover_a(6, 0, 90);
    }
    animar(CARA_NORMAL, 200);
    juego_doble_parpadeo();
}

static void juego_enojado(void)
{
    poner_led(LED_ENOJADA);
    cara_t d = CARA_NORMAL;
    d.enojo = 1;
    d.abre_izq = d.abre_der = 0.8f;
    d.y = 2;
    animar(d, 250);
    for (int i = 0; i < 6; i++) {
        mover_a(-3, 2, 50);
        mover_a(3, 2, 50);
    }
    mover_a(0, 2, 50);
    esperar(azar(800, 1500));
    animar(CARA_NORMAL, 300);
}

static void juego_llorar(void)
{
    poner_led(LED_TRISTE);
    cara_t d = CARA_NORMAL;
    d.triste = 1;
    d.abre_izq = d.abre_der = 0.75f;
    d.y = 3;
    animar(d, 400);
    int lagrimas = azar(2, 4);
    for (int i = 0; i < lagrimas; i++) {
        cara.lagrima = 0;
        d.lagrima = 1;
        animar(d, 900);
        d.lagrima = 0;
        mover_a(0, 1, 100);
        mover_a(0, 3, 100);
    }
    cara.lagrima = 0;
    esperar(500);
    animar(CARA_NORMAL, 400);
}

static void juego_enamorado(void)
{
    poner_led(LED_ENAMORADA);
    cara_t d = CARA_NORMAL;
    d.corazon = 1;
    animar(d, 100);
    for (int i = 0; i < 12; i++) {
        bool grande = (i % 2 == 0);
        d.ancho = grande ? OJO_ANCHO + 6 : OJO_ANCHO - 2;
        d.x     = grande ? -4 : 4;
        if (i % 4 == 0) cara.corazoncitos = 0;
        d.corazoncitos = (i % 4 + 1) / 4.0f;
        animar(d, grande ? 120 : 200);
    }
    cara.corazoncitos = 0;
    animar(CARA_NORMAL, 150);
    juego_doble_parpadeo();
}

static void juego_verano(void)
{
    poner_led(LED_VERANO);
    cara_t d = CARA_NORMAL;
    animar(d, 150);
    d.lentes = 1;
    d.abre_izq = d.abre_der = 0.55f;
    animar(d, 500);
    esperar(300);
    for (int i = 0; i < 4; i++) {
        mover_a(-6, 2, 300);
        mover_a(6, -2, 300);
    }
    mover_a(0, 0, 200);
    esperar(500);
    d = cara;
    d.lentes = 0;
    d.abre_izq = d.abre_der = 1;
    animar(d, 400);
    poner_led(LED_GUINO);                    // termina guiñando un ojo
    d.feliz = 1;
    animar(d, 150);
    cerrar_y_abrir(false, true, 300);
    animar(CARA_NORMAL, 200);
}

typedef struct {
    const char *nombre;
    void (*hacer)(void);
    int peso;   // mas peso = sale mas seguido
} juego_t;

static const juego_t juegos[] = {
    { "mirar",          juego_mirar,          10 },
    { "centro",         juego_centro,          4 },
    { "doble parpadeo", juego_doble_parpadeo,  3 },
    { "guino",          juego_guino,           2 },
    { "sorpresa",       juego_sorpresa,        2 },
    { "contento",       juego_contento,        2 },
    { "girar",          juego_girar,           1 },
    { "sospecha",       juego_sospecha,        1 },
    { "sueno",          juego_sueno,           1 },
    { "pasear",         juego_pasear,          2 },
    { "mareado",        juego_mareado,         1 },
    { "enojado",        juego_enojado,         1 },
    { "llorar",         juego_llorar,          1 },
    { "enamorado",      juego_enamorado,       2 },
    { "verano",         juego_verano,          2 },
};
#define CANT_JUEGOS ((int)(sizeof(juegos) / sizeof(juegos[0])))

// ─── Funciones publicas ──────────────────────────────────────────────────────

void eyes_init(color_t ojos, color_t pestanas_color, color_t boca, color_t texto, color_t fondo)
{
    color_ojos     = ojos;
    color_pestanas = pestanas_color;
    color_boca     = boca;
    color_texto    = texto;
    color_fondo    = fondo;
    color_fondo_normal = fondo;
    preparar_vueltitas();
    eyes_reiniciar();
}

void eyes_set_interrupcion(bool (*funcion)(void))
{
    hay_que_cortar = funcion;
}

void eyes_reiniciar(void)
{
    cortado = false;
    arcoiris = false;
    globo = NULL;
    comida_actual = COMIDA_NINGUNA;
    mordidas = 0;
    personaje_actual = PERSONAJE_NINGUNO;
    color_fondo = color_fondo_normal;
    cara = CARA_NORMAL;
    led_puesto = (color_led_t){ 1, 2, 3 };   // por si otro cambio el LED (el menu): que se mande de nuevo
    poner_led(LED_TRANQUILA);
    dibujar();
}

void eyes_parpadear(void)
{
    cortado = false;
    cerrar_y_abrir(true, true, 40);
}

void eyes_jugar(void)
{
    cortado = false;
    eyes_sacar_disfraz();
    int total = 0;
    for (int i = 0; i < CANT_JUEGOS; i++) total += juegos[i].peso;

    int r = azar(0, total - 1);
    for (int i = 0; i < CANT_JUEGOS; i++) {
        if (r < juegos[i].peso) {
            ESP_LOGI(TAG, "Juego: %s", juegos[i].nombre);
            juegos[i].hacer();
            poner_led(LED_TRANQUILA);
            return;
        }
        r -= juegos[i].peso;
    }
}

// La cara "alejada": chiquita, a la izquierda, con lugar a la derecha
static cara_t cara_lejos(void)
{
    cara_t d = cara;
    d.x = LEJOS_X;
    d.y = 0;
    d.lejos = 1;
    return d;
}

void eyes_onditas(void)
{
    cortado = false;
    eyes_sacar_disfraz();
    cara_t d = cara_lejos();
    d.texto = 0;
    d.ondas = 1;
    for (int i = 0; i < 4; i++) {
        d.fase += 0.25f;
        animar(d, 300);
    }
}

void eyes_decir(const char *texto, int escala, int ms)
{
    cortado = false;
    eyes_sacar_disfraz();
    snprintf(texto_costado, sizeof(texto_costado), "%s", texto);

    // Si no entra al costado, usa letras mas chicas
    while (escala > 1 && (display_text_width(texto_costado, escala) > COSTADO_ANCHO ||
                          display_text_height(texto_costado, escala) > LCD_V_RES - 8))
        escala--;
    texto_escala = escala;

    // La cara se aleja hacia la izquierda y el texto entra por la derecha
    cara_t d = cara_lejos();
    d.ondas = 0;
    d.texto = 1;
    animar(d, 500);
    esperar(ms / 2);
    if (!cortado) cerrar_y_abrir(true, true, 40);   // un parpadeo, para que se vea viva
    esperar(ms / 2);

    // El texto se va y la cara vuelve
    d = cara;
    d.x = 0;
    d.lejos = 0;
    d.texto = 0;
    animar(d, 400);
}

// ─── Reacciones con arco iris ────────────────────────────────────────────────

void eyes_saludar(void)
{
    cortado = false;
    eyes_sacar_disfraz();
    ESP_LOGI(TAG, "¡Hola! Saludando");
    arcoiris = true;
    animar(CARA_NORMAL, 150);
    juego_sorpresa();
    juego_contento();
    arcoiris = false;
    poner_led(LED_TRANQUILA);
}

void eyes_decir_que_no(void)
{
    cortado = false;
    eyes_sacar_disfraz();
    ESP_LOGI(TAG, "Estoy llena");
    arcoiris = true;
    cara_t d = CARA_NORMAL;
    d.sospecha = 1;
    animar(d, 200);
    for (int i = 0; i < 3; i++) {            // no, no, no
        mover_a(-10, 0, 110);
        mover_a(10, 0, 110);
    }
    mover_a(0, 0, 110);
    if (!cortado) eyes_decir("No,\ngracias!\nEstoy\nllena", 2, 2500);
    animar(CARA_NORMAL, 300);
    arcoiris = false;
    poner_led(LED_TRANQUILA);
}

// La comida entra, se acerca a la boca y Flufi le da 3 mordiscos (o 3 sorbos)
static void comer_o_tomar(comida_t c)
{
    cortado = false;
    eyes_sacar_disfraz();
    bool bebida = comida_es_bebida(c);
    arcoiris = true;

    // Flufi se corre un poco a la izquierda; la comida entra por la derecha
    cara_t d = CARA_NORMAL;
    d.x = -18;
    d.comida_x = 160;
    d.comida_y = 96;
    cara.comida_x = d.comida_x;
    cara.comida_y = d.comida_y;
    mordidas = 0;
    globo = NULL;
    comida_actual = c;
    animar(d, 300);

    d.comida_x = 104;                        // 1. llega
    d.asombro = 0.5f;
    animar(d, 500);
    globo = bebida ? "Sed!" : "Mmm!";
    esperar(600);
    globo = NULL;

    float boca_x = LCD_H_RES / 2.0f + d.x;   // 2. se acerca a la boca
    d.comida_x = boca_x + 26;
    d.comida_y = 100;
    d.asombro = bebida ? 0.35f : 1;
    animar(d, 400);

    for (int m = 1; m <= 3; m++) {           // 3. mordiscos o sorbos
        if (bebida) {
            globo = (c == COMIDA_GASEOSA && m == 3) ? "Burp!" : "Glu";
            d.comida_y = 96;
            animar(d, 200);
            mordidas = m;
            d.comida_y = 100;
            animar(d, 200);
            globo = NULL;
        } else {
            d.asombro = 0;
            animar(d, 120);
            if (m < 3) mordidas = m;
            else       comida_actual = COMIDA_NINGUNA;
            d.asombro = (m < 3) ? 1 : 0;
            animar(d, 160);
        }
        esperar(150);
    }

    if (bebida) {                            // se llevan el vaso vacio
        d.comida_x = 160;
        d.asombro = 0;
        animar(d, 400);
    } else {                                 // mastica
        for (int i = 0; i < 4; i++) {
            d.asombro = 0.45f; d.y = -1; animar(d, 110);
            d.asombro = 0;     d.y = 0;  animar(d, 110);
        }
    }
    comida_actual = COMIDA_NINGUNA;

    d.feliz = 1;                             // 4. ¡rico!
    globo = bebida ? "Ahh!" : "¡Rico!";
    animar(d, 250);
    esperar(1200);
    globo = NULL;
    animar(CARA_NORMAL, 400);
    arcoiris = false;
    poner_led(LED_TRANQUILA);
}

void eyes_comer(comida_t comida)
{
    if (comida == COMIDA_NINGUNA || comida_es_bebida(comida)) return;
    ESP_LOGI(TAG, "¡A comer!");
    comer_o_tomar(comida);
}

void eyes_tomar(comida_t bebida)
{
    if (!comida_es_bebida(bebida)) return;
    ESP_LOGI(TAG, "¡A tomar!");
    comer_o_tomar(bebida);
}
