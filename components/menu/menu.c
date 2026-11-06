#include "menu.h"
#include "botones.h"
#include "board_config.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "MENU";

/*
 *  Asi se ve el menu (128 x 128 pixeles, a color).
 *
 *  Sin titulo (el menu principal): entran 6 opciones
 *   ┌──────────────────────────┐
 *   │▐ Modo ██████████████████▌│  ← opcion marcada: barra rosa
 *   │  Comer                   │
 *   │  Tomar                   │
 *   │  Hora                    │
 *   │  Sensor                  │
 *   │  Salir                   │
 *   └──────────────────────────┘
 *
 *  Con titulo (por ejemplo "Modo"): entran 5 opciones
 *   ┌──────────────────────────┐
 *   │          Modo         ▲▼ │  ← titulo y flechitas si hay mas opciones
 *   ├──────────────────────────┤
 *   │▐ Atras █████████████████▌│
 *   └──────────────────────────┘
 */
#define REVISAR_CADA_MS 20

static color_t c_fondo = NEGRO, c_texto = BLANCO, c_barra = COLOR(255, 120, 190), c_texto_barra = NEGRO;

void menu_colores(color_t fondo, color_t texto, color_t barra, color_t texto_barra)
{
    c_fondo = fondo;
    c_texto = texto;
    c_barra = barra;
    c_texto_barra = texto_barra;
}

// Como se acomodan las filas, segun haya titulo o no
typedef struct {
    bool con_titulo;
    int  filas;        // cuantas opciones entran
    int  alto_fila;
    int  y_primera;    // altura de la primera fila
} disposicion_t;

static disposicion_t disposicion(const char *titulo)
{
    if (titulo != NULL && titulo[0] != '\0')
        return (disposicion_t){ .con_titulo = true,  .filas = 5, .alto_fila = 20, .y_primera = 26 };
    return     (disposicion_t){ .con_titulo = false, .filas = 6, .alto_fila = 21, .y_primera = 1 };
}

static void dibujar_titulo(const char *titulo)
{
    display_draw_text_centered(titulo, 4, 2, c_barra);
    display_fill_rect(0, 21, LCD_H_RES, 2, c_barra);
}

static void dibujar(const char *titulo, const char *const opciones[], int cantidad,
                    int elegida, int destacada, int primera_visible)
{
    disposicion_t d = disposicion(titulo);
    bool hay_arriba = primera_visible > 0;
    bool hay_abajo  = primera_visible + d.filas < cantidad;

    display_clear(c_fondo);
    if (d.con_titulo) dibujar_titulo(titulo);

    // Flechitas en el borde derecho si hay mas opciones arriba o abajo
    int arriba = d.y_primera, abajo = d.y_primera + d.filas * d.alto_fila - 2;
    if (hay_arriba) display_fill_triangle(120, arriba + 6, 127, arriba + 6, 123, arriba, c_texto);
    if (hay_abajo)  display_fill_triangle(120, abajo - 6, 127, abajo - 6, 123, abajo, c_texto);
    int ancho_barra = (hay_arriba || hay_abajo) ? LCD_H_RES - 10 : LCD_H_RES - 2;

    for (int fila = 0; fila < d.filas; fila++) {
        int i = primera_visible + fila;
        if (i >= cantidad) break;

        int y = d.y_primera + fila * d.alto_fila;
        bool marcada = (i == elegida);
        if (marcada) display_fill_round_rect(1, y, ancho_barra, d.alto_fila - 2, 6, c_barra);

        display_draw_text(8, y + (d.alto_fila - 2 - 14) / 2, opciones[i], 2, marcada ? c_texto_barra : c_texto);
        if (i == destacada)
            display_fill_circle(ancho_barra - 8, y + (d.alto_fila - 2) / 2, 3, marcada ? c_texto_barra : c_barra);
    }

    display_flush();
}

// Espera una apretada: B = siguiente, A = elegir, BOOT = atras.
// Devuelve la opcion elegida, MENU_ATRAS o MENU_SE_CERRO.
static int esperar_eleccion(const char *nombre, int cantidad, int *elegida, int espera_max_ms,
                            void (*redibujar)(void *datos), void *datos)
{
    botones_olvidar();              // apretadas viejas no cuentan
    int quieto_ms = 0;
    bool hay_que_dibujar = true;

    while (quieto_ms < espera_max_ms) {
        if (botones_se_apreto(BOTON_B)) {
            *elegida = (*elegida + 1) % cantidad;
            quieto_ms = 0;
            hay_que_dibujar = true;
        }
        if (botones_se_apreto(BOTON_A)) {
            ESP_LOGI(TAG, "%s → opcion %d", nombre, *elegida);
            return *elegida;
        }
        if (botones_se_apreto(BOTON_BOOT)) {
            ESP_LOGI(TAG, "%s → atras", nombre);
            return MENU_ATRAS;
        }
        if (hay_que_dibujar) {
            redibujar(datos);
            hay_que_dibujar = false;
        }
        vTaskDelay(pdMS_TO_TICKS(REVISAR_CADA_MS));
        quieto_ms += REVISAR_CADA_MS;
    }

    ESP_LOGI(TAG, "%s: nadie eligio nada, se cierra solo", nombre);
    return MENU_SE_CERRO;
}

// ─── Menu de lista ───────────────────────────────────────────────────────────

typedef struct {
    const char *titulo;
    const char *const *opciones;
    int cantidad, *elegida, destacada, primera_visible;
} lista_t;

static void redibujar_lista(void *datos)
{
    lista_t *l = datos;
    disposicion_t d = disposicion(l->titulo);
    // Correr la "ventana" para que la opcion marcada siempre se vea
    if (*l->elegida < l->primera_visible) l->primera_visible = *l->elegida;
    if (*l->elegida >= l->primera_visible + d.filas) l->primera_visible = *l->elegida - d.filas + 1;
    dibujar(l->titulo, l->opciones, l->cantidad, *l->elegida, l->destacada, l->primera_visible);
}

int menu_elegir(const char *titulo, const char *const opciones[], int cantidad,
                int elegida, int destacada, int espera_max_ms)
{
    if (elegida < 0 || elegida >= cantidad) elegida = 0;
    lista_t l = { titulo, opciones, cantidad, &elegida, destacada, 0 };
    const char *nombre = (titulo != NULL && titulo[0] != '\0') ? titulo : "Menu";
    return esperar_eleccion(nombre, cantidad, &elegida, espera_max_ms, redibujar_lista, &l);
}

// ─── Menu con dibujos ────────────────────────────────────────────────────────

typedef struct {
    const char *titulo;
    int cantidad, *elegida;
    void (*dibujar_opcion)(int);
} dibujos_t;

static void redibujar_dibujos(void *datos)
{
    dibujos_t *m = datos;
    display_clear(c_fondo);
    dibujar_titulo(m->titulo);

    // Puntitos abajo: uno por opcion, el de la opcion actual relleno
    int x0 = LCD_H_RES / 2 - (m->cantidad - 1) * 5;
    for (int i = 0; i < m->cantidad; i++) {
        display_fill_circle(x0 + i * 10, 123, 3, c_barra);
        if (i != *m->elegida) display_fill_circle(x0 + i * 10, 123, 2, c_fondo);
    }

    m->dibujar_opcion(*m->elegida);
    display_flush();
}

int menu_elegir_dibujos(const char *titulo, int cantidad, void (*dibujar_opcion)(int opcion),
                        int elegida, int espera_max_ms)
{
    if (elegida < 0 || elegida >= cantidad) elegida = 0;
    dibujos_t m = { titulo, cantidad, &elegida, dibujar_opcion };
    return esperar_eleccion(titulo, cantidad, &elegida, espera_max_ms, redibujar_dibujos, &m);
}
