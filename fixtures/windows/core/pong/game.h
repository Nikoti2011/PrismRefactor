#ifndef PRISM_PONG_GAME_H
#define PRISM_PONG_GAME_H
/* Integer fixed-step state shared by the Windows fixture and host logic checks. */
typedef struct { int paddle, other, x, y, vx, vy, left, right; } Game;
static void reset_ball(Game *g) { g->x=160; g->y=120; g->vx=2; g->vy=1; }
static void restart(Game *g) { g->paddle=100; g->other=100; g->left=0; g->right=0; reset_ball(g); }
static void step(Game *g, int up, int down) {
    g->paddle += (down-up)*3;
    if(g->paddle<0) g->paddle=0;
    if(g->paddle>200) g->paddle=200;
    if(g->other+20<g->y) g->other+=2;
    if(g->other+20>g->y) g->other-=2;
    if(g->other<0) g->other=0;
    if(g->other>200) g->other=200;
    g->x+=g->vx; g->y+=g->vy;
    if(g->y<=2) {g->y=2; g->vy=1;}
    if(g->y>=237) {g->y=237; g->vy=-1;}
    if(g->vx<0 && g->x<=16 && g->x>=12 && g->y>=g->paddle && g->y<g->paddle+40) g->vx=2;
    if(g->vx>0 && g->x>=303 && g->x<=307 && g->y>=g->other && g->y<g->other+40) g->vx=-2;
    if(g->x<0) {g->right=(g->right+1)%10; reset_ball(g);}
    if(g->x>=320) {g->left=(g->left+1)%10; reset_ball(g);}
}
#endif
