#include "../../fixtures/windows/core/pong/game.h"
#include <stdio.h>
#define CHECK(x) do { if(!(x)) {fprintf(stderr,"line %d: %s\n",__LINE__,#x); return 1;} } while(0)
int main(void) {
    Game g;
    restart(&g); CHECK(g.x==160 && g.left==0 && g.right==0);
    for(int i=0;i<100;i++) step(&g,1,0);
    CHECK(g.paddle==0);
    for(int i=0;i<100;i++) step(&g,0,1);
    CHECK(g.paddle==200);
    restart(&g); g.x=17; g.y=110; g.vx=-2;
    step(&g,0,0); CHECK(g.vx==2 && g.left==0 && g.right==0);
    g.y=2; g.vy=-1; step(&g,0,0); CHECK(g.y==2 && g.vy==1);
    g.y=237; g.vy=1; step(&g,0,0); CHECK(g.y==237 && g.vy==-1);
    g.x=0; g.vx=-2; step(&g,0,0); CHECK(g.right==1 && g.x==160);
    g.x=319; g.vx=2; step(&g,0,0); CHECK(g.left==1 && g.x==160);
    restart(&g); CHECK(g.left==0 && g.right==0 && g.paddle==100);
    int before=g.paddle; step(&g,0,0); CHECK(g.paddle==before);
    for(int i=0;i<36000;i++) {
        step(&g,(i/120)%2,1-(i/120)%2);
        CHECK(g.paddle>=0 && g.paddle<=200 && g.other>=0 && g.other<=200);
        CHECK(g.x>=0 && g.x<320 && g.y>=2 && g.y<=237);
        CHECK(g.left>=0 && g.left<10 && g.right>=0 && g.right<10);
    }
    puts("PASS movement, bounds, collisions, scoring, restart, released keys, 36000 steps");
    return 0;
}
