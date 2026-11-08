// ─────────────────────────────────────────────────────────────────────────────
//  Las charlas de la familia (iguales en todos los personajes)
// ─────────────────────────────────────────────────────────────────────────────
//
//  Cada charla es una lista de renglones. El que empieza dice el primero, el
//  otro el segundo, el primero el tercero, y asi (como en una obra de teatro).
//
//  ¡Inventen charlas nuevas! Reglas para que entren en todas las pantallas:
//    - '\n' = cambiar de renglon. Hasta 4 renglones por turno.
//    - Hasta 8 letras por renglon (contando ¡ y ¿).
//    - Sin tildes ni ñ (la letra de los personajes no las tiene).
//    - Que sirva para cualquiera (no "amigo" ni "amiga": "Pitri" es nene y
//      "Flufi" es nena).
//
#include "familia.h"
#include <stddef.h>

typedef struct {
    const char *texto;
    familia_emocion_t emocion;
} renglon_t;

#define FIN { NULL, FAMILIA_EMOCION_NADA }
#define NADA     FAMILIA_EMOCION_NADA
#define CONTENTA FAMILIA_EMOCION_CONTENTA
#define AMOR     FAMILIA_EMOCION_AMOR
#define RISA     FAMILIA_EMOCION_RISA

static const renglon_t charlas[][5] = {
    { { "¡Hola!\n¿Como\nestas?", NADA },  { "¡Bien!\n¿Y vos?", CONTENTA },    { "¡Super\nbien!", CONTENTA }, FIN },
    { { "¿Jugamos\nun rato?", NADA },     { "¡Si!\n¡Dale!", CONTENTA },       { "¡Yupi!", CONTENTA }, FIN },
    { { "¿Que\ncomiste\nhoy?", NADA },    { "¡Algo\nrico!\n¿Y vos?", NADA },  { "¡Yo\ntambien!", CONTENTA }, FIN },
    { { "¡Que\nlindo\nverte!", AMOR },    { "¡Lo\nmismo\ndigo!", AMOR },      { "<3", AMOR }, FIN },
    { { "¿Sabes\nun\nchiste?", NADA },    { "No...\n¿y vos?", NADA },         { "¡Yo\ntampoco!", RISA }, { "Jaja", RISA }, FIN },
    { { "¡Que\ncalor!", NADA },           { "¡Si!\n¡Mucho!", NADA },          { "¿Vamos a\nla pile?", CONTENTA }, { "¡Dale!", CONTENTA }, FIN },
    { { "¿Vamos a\nbailar?", NADA },      { "¡Si!\n¡Musica!", CONTENTA },     { "¡A\nbailar!", CONTENTA }, FIN },
    { { "¡Te\nquiero\nmucho!", AMOR },    { "¡Y yo\na vos!", AMOR }, FIN },
};
#define CANT_CHARLAS ((int)(sizeof(charlas) / sizeof(charlas[0])))

int familia_cant_charlas(void)
{
    return CANT_CHARLAS;
}

const char *familia_charla(int charla, int renglon, familia_emocion_t *emocion)
{
    if (charla < 0 || charla >= CANT_CHARLAS || renglon < 0 || renglon >= 5) return NULL;
    const renglon_t *r = &charlas[charla][renglon];
    if (emocion) *emocion = r->emocion;
    return r->texto;
}

// ─── Juegos para hacer juntos ────────────────────────────────────────────────
// Tienen que existir (con este nombre) en los juegos de todos los personajes
static const char *const juegos_juntos[] = {
    "contento", "sorpresa", "girar", "mareado", "enamorado", "verano",
};

int familia_cant_juegos_juntos(void)
{
    return (int)(sizeof(juegos_juntos) / sizeof(juegos_juntos[0]));
}

const char *familia_juego_junto(int i)
{
    if (i < 0 || i >= familia_cant_juegos_juntos()) return NULL;
    return juegos_juntos[i];
}
