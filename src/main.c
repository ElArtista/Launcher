/*********************************************************************************************************************/
/*                                                  /===-_---~~~~~~~~~------____                                     */
/*                                                 |===-~___                _,-'                                     */
/*                  -==\\                         `//~\\   ~~~~`---.___.-~~                                          */
/*              ______-==|                         | |  \\           _-~`                                            */
/*        __--~~~  ,-/-==\\                        | |   `\        ,'                                                */
/*     _-~       /'    |  \\                      / /      \      /                                                  */
/*   .'        /       |   \\                   /' /        \   /'                                                   */
/*  /  ____  /         |    \`\.__/-~~ ~ \ _ _/'  /          \/'                                                     */
/* /-'~    ~~~~~---__  |     ~-/~         ( )   /'        _--~`                                                      */
/*                   \_|      /        _)   ;  ),   __--~~                                                           */
/*                     '~~--_/      _-~/-  / \   '-~ \                                                               */
/*                    {\__--_/}    / \\_>- )<__\      \                                                              */
/*                    /'   (_/  _-~  | |__>--<__|      |                                                             */
/*                   |0  0 _/) )-~     | |__>--<__|     |                                                            */
/*                   / /~ ,_/       / /__>---<__/      |                                                             */
/*                  o o _//        /-~_>---<__-~      /                                                              */
/*                  (^(~          /~_>---<__-      _-~                                                               */
/*                 ,/|           /__>--<__/     _-~                                                                  */
/*              ,//('(          |__>--<__|     /                  .----_                                             */
/*             ( ( '))          |__>--<__|    |                 /' _---_~\                                           */
/*          `-)) )) (           |__>--<__|    |               /'  /     ~\`\                                         */
/*         ,/,'//( (             \__>--<__\    \            /'  //        ||                                         */
/*       ,( ( ((, ))              ~-__>--<_~-_  ~--____---~' _/'/        /'                                          */
/*     `~/  )` ) ,/|                 ~-_~>--<_/-__       __-~ _/                                                     */
/*   ._-~//( )/ )) `                    ~~-'_/_/ /~~~~~~~__--~                                                       */
/*    ;'( ')/ ,)(                              ~~~~~~~~~~                                                            */
/*   ' ') '( (/                                                                                                      */
/*     '   '  `                                                                                                      */
/*********************************************************************************************************************/
#include <stdlib.h>
#include <tinycthread.h>
#include "window.h"
#include "renderer.h"

static int window_thrd(void* arg)
{
    /* Create window and context */
    struct window* wnd = (struct window*) arg;
    window_open(wnd);
    /* Initialize renderer state */
    struct renderer_state rs;
    renderer_init(&rs);
    /* Loop */
    wnd->render_data = &rs;
    window_loop(wnd);
    /* Deinitialize renderer state */
    renderer_destroy(&rs);
    return 0;
}

/* Proxy fn to avoid func ptr casting */
static void render_fn(void* arg) { renderer_render(arg); }

int main(int argc, char* argv[])
{
    (void) argc;
    (void) argv;

    /* Open window */
    struct window wnd;
    wnd.progress = 57;
    wnd.render_fn = render_fn;
    wnd.width = 640;
    wnd.height = 480;
    wnd.title = "Launcher";

    /* Launch window loop thread */
    thrd_t wnd_thrd;
    thrd_create(&wnd_thrd, window_thrd, &wnd);

    /* Wait for window loop thread to finish */
    thrd_join(wnd_thrd, 0);
    return 0;
}
