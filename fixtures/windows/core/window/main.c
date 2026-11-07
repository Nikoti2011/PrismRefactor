#include "../fixture.h"
#ifdef PRISM_PONG
#include "../pong/game.h"
static Game game;
#endif
static DWORD pixels[WIDTH*HEIGHT];
static int up, down;
static HWND window;
static const WCHAR class_name[]=L"PrismFixtureV1";
static BITMAPINFO bitmap = {{sizeof(BITMAPINFOHEADER),WIDTH,-HEIGHT,1,32,BI_RGB,WIDTH*HEIGHT*4,0,0,0,0},{{0,0,0,0}}};
static void render(void) {
    int x,y;
    HDC dc;
    for(y=0;y<HEIGHT;y++) for(x=0;x<WIDTH;x++) {
#ifdef PRISM_PONG
        DWORD color=0x00102030;
        if((x>=10 && x<15 && y>=game.paddle && y<game.paddle+40) ||
           (x>=305 && x<310 && y>=game.other && y<game.other+40) ||
           (x>=game.x-2 && x<=game.x+2 && y>=game.y-2 && y<=game.y+2)) color=0x00ffffff;
        /* Score as up to nine separated blocks on each side. */
        if(y>=8 && y<14 && ((x>=30 && x<30+game.left*8 && x%8<5) ||
           (x>=210 && x<210+game.right*8 && x%8<5))) color=0x0000ff00;
#else
        DWORD color=y<HEIGHT/2 ? (x<WIDTH/2?0x00ff0000:0x0000ff00) : (x<WIDTH/2?0x000000ff:0x00ffffff);
        if(up && y<16) color=0x00ffff00;
        if(down && y>=HEIGHT-16) color=0x0000ffff;
#endif
        pixels[y*WIDTH+x]=color;
    }
    dc=GetDC(window);
    if(!dc) ExitProcess(20);
    if(StretchDIBits(dc,0,0,WIDTH,HEIGHT,0,0,WIDTH,HEIGHT,pixels,&bitmap,DIB_RGB_COLORS,SRCCOPY)!=HEIGHT) ExitProcess(21);
    if(!ReleaseDC(window,dc)) ExitProcess(22);
}
static LRESULT CALLBACK procedure(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch(msg) {
    case WM_NCCREATE: return TRUE;
    case WM_CREATE: return 0;
    case WM_KEYDOWN:
        if(wp==VK_UP) up=1;
        if(wp==VK_DOWN) down=1;
        if(wp==VK_ESCAPE) DestroyWindow(hwnd);
#ifdef PRISM_PONG
        if(wp=='R') restart(&game);
#endif
        return 0;
    case WM_KEYUP: if(wp==VK_UP) up=0; if(wp==VK_DOWN) down=0; return 0;
    case WM_KILLFOCUS: up=down=0; return 0;
    case WM_CLOSE: DestroyWindow(hwnd); return 0;
    case WM_DESTROY: PostQuitMessage(0); return 0;
    default: return DefWindowProcW(hwnd,msg,wp,lp);
    }
}
DWORD PrismEntry(void) {
    WNDCLASSEXW cls={0};
    MSG msg;
    LARGE_INTEGER frequency, previous, now;
    RECT rect;
    cls.cbSize=sizeof(cls); cls.lpfnWndProc=procedure;
    cls.hInstance=GetModuleHandleW(0); cls.lpszClassName=class_name;
    if(!cls.hInstance || !RegisterClassExW(&cls)) return 10;
    /* Borderless fixed client area avoids requiring AdjustWindowRectEx. */
    window=CreateWindowExW(0,class_name,L"Prism fixture: arrows, Esc, R",WS_POPUP,100,100,WIDTH,HEIGHT,0,0,cls.hInstance,0);
    if(!window) return 11;
    if(!GetClientRect(window,&rect) || rect.right!=WIDTH || rect.bottom!=HEIGHT) return 12;
    ShowWindow(window,SW_SHOW);
    if(!QueryPerformanceFrequency(&frequency) || frequency.QuadPart<=0 || !QueryPerformanceCounter(&previous)) return 13;
#ifdef PRISM_PONG
    restart(&game);
#endif
    OutputDebugStringA("Prism window ready\n");
    for(;;) {
        while(PeekMessageW(&msg,0,0,0,PM_REMOVE)) {
            if(msg.message==WM_QUIT) return (DWORD)msg.wParam;
            TranslateMessage(&msg); DispatchMessageW(&msg);
        }
        if(!QueryPerformanceCounter(&now)) return 14;
        if(now.QuadPart-previous.QuadPart>=frequency.QuadPart/60) {
#ifdef PRISM_PONG
            step(&game,up,down);
#endif
            previous=now; render();
        }
        Sleep(1);
    }
}
