#include <c64.h>
#include <cbm.h>
#include <conio.h>
#include <string.h>
#include <stdlib.h>

#define SCREEN ((unsigned char*)0x8c00)
#define COLOR  ((unsigned char*)0xd800)
#define VIC_SPRITE_X (*(volatile unsigned char*)0xd000)
#define VIC_SPRITE_Y (*(volatile unsigned char*)0xd001)
#define VIC_SPRITE_EN (*(volatile unsigned char*)0xd015)
#define VIC_SPRITE_COL (*(volatile unsigned char*)0xd027)
#define VIC_SPRITE_PTR (*(volatile unsigned char*)0x8ff8)
#define JOY2 (*(volatile unsigned char*)0xdc00)
#define BITMAP ((unsigned char*)0x6000)
#define BMCOLOR ((unsigned char*)0x5c00)
#define FONT ((unsigned char*)0x9800)

#define BLACK 0
#define WHITE 1
#define RED 2
#define CYAN 3
#define PURPLE 4
#define GREEN 5
#define BLUE 6
#define YELLOW 7
#define ORANGE 8
#define LTRED 10
#define DARKGREY 11
#define GREY 12
#define LTGREEN 13
#define LTBLUE 14
#define LTGREY 15

#define GL_TL 27
#define GL_TR 28
#define GL_BL 29
#define GL_BR 30
#define GL_HZ 31
#define GL_VT 94
#define GL_MARK 95
#define GL_HAZ1 96
#define GL_HAZ2 97
#define GL_NODE 98
#define GL_WIRE 99
#define GL_VENT 100
#define GL_RIVET 101
#define GL_CHEV 102
#define GL_BARS 103

#define MAX_EVENTS 60

typedef struct {
    char date[9];
    char time[5];
    char note[29];
} Event;

extern void __fastcall__ install_graphics(void);

static Event events[MAX_EVENTS];
static unsigned char event_count;
static unsigned char dirty;
static unsigned int year = 2026;
static unsigned char month = 8;
static unsigned char day = 31;
static char today[9] = "20260831";
static const char* months[] = {
    "JANUARY", "FEBRUARY", "MARCH", "APRIL", "MAY", "JUNE",
    "JULY", "AUGUST", "SEPTEMBER", "OCTOBER", "NOVEMBER", "DECEMBER"
};

static void draw_calendar(void);
static void move_day(signed char delta);

static void sfx(unsigned int freq,unsigned char wave,unsigned char delay) {
    unsigned int n;
    SID.v1.ctrl=0; SID.v1.freq=freq; SID.v1.ad=0x02; SID.v1.sr=0x80;
    SID.amp=0x0f; SID.v1.ctrl=(unsigned char)(wave|1);
    for(n=0;n<(unsigned int)delay*90u;++n);
    SID.v1.ctrl=wave; SID.amp=0;
}

static void screen_beep(void) { sfx(0x1c00,0x10,2); }
static void sfx_move(void) { sfx(0x1700,0x10,2); }
static void sfx_ok(void) { sfx(0x2600,0x10,3); }

static unsigned char screen_code(char c) {
    unsigned char u=(unsigned char)c;
    if(u>=0xc1&&u<=0xda) return (unsigned char)(u-0xc0);
    if(u>=0x41&&u<=0x5a) return (unsigned char)(u-0x40);
    if(u>=0x61&&u<=0x7a) return (unsigned char)(u-0x60);
    if(u>=0x20&&u<=0x3f) return u;
    return 32;
}

static void bitmap_mode(void) {
    VIC_SPRITE_EN=0;
    *(unsigned char*)0xdd00=(unsigned char)((*(unsigned char*)0xdd00&0xfc)|2);
    *(unsigned char*)0xd018=0x78; *(unsigned char*)0xd011|=0x20;
}

static void char_mode(void) {
    *(unsigned char*)0xd011&=0xdf;
    *(unsigned char*)0xdd00=(unsigned char)((*(unsigned char*)0xdd00&0xfc)|1);
    *(unsigned char*)0xd018=0x36;
}

static unsigned int bmoff(unsigned char cx,unsigned char cy) {
    return (unsigned int)cy*320u+(unsigned int)cx*8u;
}

static void bm_cell(unsigned char cx,unsigned char cy,unsigned char value) {
    unsigned int p=bmoff(cx,cy); unsigned char r;
    for(r=0;r<8;++r) BITMAP[p+r]=value;
}

static void bm_char(unsigned char cx,unsigned char cy,unsigned char ch,unsigned char inv) {
    unsigned int p=bmoff(cx,cy); unsigned int f=(unsigned int)ch*8u; unsigned char r,v;
    for(r=0;r<8;++r) { v=FONT[f+r]; BITMAP[p+r]=inv?(unsigned char)~v:v; }
}

static void bm_text(unsigned char x,unsigned char y,const char* s,unsigned char inv) {
    while(*s&&x<40) bm_char(x++,y,screen_code(*s++),inv);
}

static void bm_cells(unsigned char x,unsigned char y,unsigned char w,unsigned char h,unsigned char v) {
    unsigned char xx,yy; for(yy=0;yy<h;++yy) for(xx=0;xx<w;++xx) bm_cell(x+xx,y+yy,v);
}

static void bm_color(unsigned char x,unsigned char y,unsigned char w,unsigned char h,unsigned char col) {
    unsigned char xx,yy; for(yy=0;yy<h;++yy) for(xx=0;xx<w;++xx) BMCOLOR[(unsigned int)(y+yy)*40u+x+xx]=(unsigned char)(col<<4);
}

static void bm_hline(unsigned char x,unsigned char y,unsigned char w) {
    unsigned int row=(unsigned int)(y>>3)*320u+(y&7); unsigned char i;
    for(i=0;i<w;++i) BITMAP[row+(unsigned int)(x+i)*8u]=0xff;
}

static void bm_vline(unsigned char x,unsigned char y,unsigned char h) {
    unsigned char i,mask=(unsigned char)(0x80>>(x&7)); unsigned int p;
    for(i=0;i<h;++i) { p=(unsigned int)((y+i)>>3)*320u+(unsigned int)(x>>3)*8u+((y+i)&7); BITMAP[p]|=mask; }
}

static void bm_dot(unsigned int x,unsigned char y,unsigned char on) {
    unsigned int p=(unsigned int)(y>>3)*320u+(unsigned int)(x>>3)*8u+(y&7); unsigned char m=(unsigned char)(0x80>>(x&7));
    if(on) BITMAP[p]|=m; else BITMAP[p]&=(unsigned char)~m;
}

static void bm_line(int x0,int y0,int x1,int y1,unsigned char on) {
    int dx=abs(x1-x0),sx=x0<x1?1:-1,dy=-abs(y1-y0),sy=y0<y1?1:-1,er=dx+dy,e;
    while(1) {
        bm_dot((unsigned int)x0,(unsigned char)y0,on);
        if(x0==x1&&y0==y1) break;
        e=er<<1;
        if(e>=dy) { er+=dy; x0+=sx; }
        if(e<=dx) { er+=dx; y0+=sy; }
    }
}

static void bm_stroke(int x0,int y0,int x1,int y1,unsigned char on) {
    bm_line(x0,y0,x1,y1,on); bm_line(x0,y0+1,x1,y1+1,on);
    bm_line(x0,y0+2,x1,y1+2,on);
}

static void bm_cyber_logo(void) {
    static const unsigned char glyph[9][7]={
        {14,17,16,16,16,17,14}, /* C */
        {17,17,10,4,4,4,4},     /* Y */
        {30,17,17,30,17,17,30}, /* B */
        {31,16,16,30,16,16,31}, /* E */
        {30,17,17,30,20,18,17}, /* R */
        {14,17,17,31,17,17,17}, /* A */
        {14,17,16,23,17,17,14}, /* G */
        {17,25,21,21,19,17,17}, /* N */
        {30,17,17,17,17,17,30}  /* D */
    };
    static const unsigned char word[11]={0,1,2,3,4,5,6,3,7,8,5};
    unsigned char a,r,b; unsigned int x;
    /* CYBERAGENDA: compact hand-pixelled alphabet, never character ROM text. */
    bm_cells(24,3,9,2,0); bm_color(24,3,9,2,RED);
    for(a=0;a<11;++a) {
        x=(unsigned int)(194+a*6);
        for(r=0;r<7;++r) for(b=0;b<5;++b)
            if(glyph[word[a]][r]&(unsigned char)(16>>b)) {
                bm_dot(x+b,(unsigned char)(26+r*2),1);
                bm_dot(x+b,(unsigned char)(27+r*2),1);
            }
    }
    bm_line(192,24,259,24,1); bm_line(198,41,261,38,1); bm_line(254,22,262,27,1);
}

static void cell(unsigned char x, unsigned char y, unsigned char ch, unsigned char col) {
    unsigned int p = (unsigned int)y * 40u + x;
    SCREEN[p] = ch;
    COLOR[p] = col;
}

static void text(unsigned char x, unsigned char y, const char* s, unsigned char col) {
    while (*s && x < 40) cell(x++, y, screen_code(*s++), col);
}

static void inv_text(unsigned char x, unsigned char y, const char* s, unsigned char col) {
    while (*s && x < 40) cell(x++, y, (unsigned char)(screen_code(*s++)+128), col);
}

static void fill(unsigned char x, unsigned char y, unsigned char w, unsigned char h,
                 unsigned char ch, unsigned char col) {
    unsigned char xx, yy;
    for (yy = 0; yy < h; ++yy)
        for (xx = 0; xx < w; ++xx) cell(x + xx, y + yy, ch, col);
}

static void box(unsigned char x, unsigned char y, unsigned char w, unsigned char h,
                unsigned char col) {
    unsigned char i;
    cell(x, y, GL_TL, col); cell(x+w-1, y, GL_TR, col);
    cell(x, y+h-1, GL_BL, col); cell(x+w-1, y+h-1, GL_BR, col);
    for (i=1; i<w-1; ++i) { cell(x+i,y,GL_HZ,col); cell(x+i,y+h-1,GL_HZ,col); }
    for (i=1; i<h-1; ++i) { cell(x,y+i,GL_VT,col); cell(x+w-1,y+i,GL_VT,col); }
}

static void num2(char* out, unsigned char n) {
    out[0] = (char)('0' + n/10); out[1] = (char)('0' + n%10); out[2] = 0;
}

static void num4(char* out, unsigned int n) {
    out[0]=(char)('0'+(n/1000)%10); out[1]=(char)('0'+(n/100)%10);
    out[2]=(char)('0'+(n/10)%10); out[3]=(char)('0'+n%10); out[4]=0;
}

static unsigned char parse2(const char* s) {
    return (unsigned char)((s[0]-'0')*10 + (s[1]-'0'));
}

static unsigned int parse4(const char* s) {
    return (unsigned int)((s[0]-'0')*1000u + (s[1]-'0')*100u +
                          (s[2]-'0')*10u + (s[3]-'0'));
}

static unsigned char leap(unsigned int y) {
    return (unsigned char)((y%4==0 && y%100!=0) || y%400==0);
}

static unsigned char days_in_month(unsigned int y, unsigned char m) {
    static const unsigned char d[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    if (m==2 && leap(y)) return 29;
    return d[m-1];
}

static unsigned char first_weekday(unsigned int y, unsigned char m) {
    unsigned int yy=y; unsigned char mm=m; unsigned long v;
    if (mm<3) { mm+=12; --yy; }
    v=1UL + (13UL*(mm+1))/5UL + yy + yy/4 - yy/100 + yy/400;
    return (unsigned char)((v+6)%7);
}

static unsigned char weekday_of(unsigned int y,unsigned char m,unsigned char d) {
    return (unsigned char)((first_weekday(y,m)+d-1)%7);
}

static unsigned char nth_weekday(unsigned int y,unsigned char m,unsigned char wd,unsigned char nth) {
    unsigned char first=first_weekday(y,m);
    return (unsigned char)(1+((wd+7-first)%7)+(nth-1)*7);
}

static unsigned char last_weekday(unsigned int y,unsigned char m,unsigned char wd) {
    unsigned char d=days_in_month(y,m);
    return (unsigned char)(d-((weekday_of(y,m,d)+7-wd)%7));
}

static const char* holiday_name(unsigned int y,unsigned char m,unsigned char d) {
    if(m==1&&d==1) return "NEW YEAR";
    if(m==1&&d==nth_weekday(y,1,1,3)) return "MLK DAY";
    if(m==2&&d==nth_weekday(y,2,1,3)) return "PRESIDENTS";
    if(m==5&&d==last_weekday(y,5,1)) return "MEMORIAL";
    if(m==6&&d==19) return "JUNETEENTH";
    if(m==7&&d==4) return "JULY 4TH";
    if(m==9&&d==nth_weekday(y,9,1,1)) return "LABOR DAY";
    if(m==10&&d==nth_weekday(y,10,1,2)) return "COLUMBUS";
    if(m==11&&d==11) return "VETERANS";
    if(m==11&&d==nth_weekday(y,11,4,4)) return "THANKSGIV";
    if(m==12&&d==25) return "CHRISTMAS";
    return 0;
}

static void make_date(char* out, unsigned int y, unsigned char m, unsigned char d) {
    num4(out,y); num2(out+4,m); num2(out+6,d); out[8]=0;
}

static unsigned char event_on(unsigned char d) {
    char date[9]; unsigned char i;
    make_date(date,year,month,d);
    for(i=0;i<event_count;++i) if(!strcmp(events[i].date,date)) return 1;
    return 0;
}

static unsigned char first_event_on(unsigned char d) {
    char date[9]; unsigned char i;
    make_date(date,year,month,d);
    for(i=0;i<event_count;++i) if(!strcmp(events[i].date,date)) return i;
    return 255;
}

static void draw_day_cell(unsigned char d,unsigned char selected) {
    unsigned char idx=(unsigned char)(first_weekday(year,month)+d-1);
    unsigned char x=(unsigned char)(2+(idx%7)*4),y=(unsigned char)(8+(idx/7)*2);
    char n[3];
    bm_cells(x,y,3,1,0); bm_color(x,y,3,1,YELLOW); num2(n,d);
    if(selected) bm_color(x,y,2,1,CYAN);
    else if(holiday_name(year,month,d)) bm_color(x,y,2,1,LTRED);
    bm_text(x,y,n,0);
    if(event_on(d)) { bm_color((unsigned char)(x+2),y,1,1,LTRED); bm_char((unsigned char)(x+2),y,GL_NODE,0); }
}

static void draw_day_feed(void) {
    unsigned char ix,row=14; char n[9]; const char* h;
    bm_cells(30,11,9,7,0xff); bm_color(30,11,9,7,LTGREY);
    bm_color(35,9,1,1,dirty?LTRED:CYAN); bm_char(35,9,GL_NODE,0);
    make_date(n,year,month,day); bm_text(30,11,n,1);
    h=holiday_name(year,month,day); if(h) bm_text(30,12,h,1);
    for(ix=0;ix<event_count&&row<18;++ix) if(!strcmp(events[ix].date,n)) {
        bm_text(30,row,events[ix].time,1); bm_text(30,(unsigned char)(row+1),events[ix].note,1); row+=3;
    }
    if(row==14) bm_text(30,14,"NO EVENTS",1);
}

static void draw_month_panel(void) {
    static const char* wd[] = {"SU","MO","TU","WE","TH","FR","SA"};
    unsigned char i,d,dim=days_in_month(year,month); char n[5];
    /* Preserve the case and frame; replace only the live calendar surface. */
    bm_cells(2,6,26,15,0); bm_color(2,6,26,15,YELLOW);
    bm_text(2,6,"<",0); bm_text(5,6,months[month-1],0);
    num4(n,year); bm_text(22,6,n,0); bm_text(27,6,">",0);
    for(i=0;i<7;++i) bm_text((unsigned char)(2+i*4),7,wd[i],0);
    for(d=1;d<=dim;++d) draw_day_cell(d,(unsigned char)(d==day));
}

static void refresh_dynamic(void) {
    bitmap_mode(); draw_month_panel(); draw_day_feed();
}

static void move_selection(signed char delta) {
    unsigned int oldyear=year; unsigned char oldmonth=month,oldday=day;
    move_day(delta);
    if(year!=oldyear||month!=oldmonth) { draw_month_panel(); draw_day_feed(); }
    else { draw_day_cell(oldday,0); draw_day_cell(day,1); draw_day_feed(); }
}

static void draw_calendar(void) {
    unsigned char i; unsigned int px;
    bitmap_mode();
    for(i=0;i<250;++i) { BITMAP[i]=0; BITMAP[i+250]=0; BITMAP[i+500]=0; BITMAP[i+750]=0; }
    for(i=0;i<250;++i) { BITMAP[i+1000]=0; BITMAP[i+1250]=0; BITMAP[i+1500]=0; BITMAP[i+1750]=0; }
    for(i=0;i<250;++i) { BITMAP[i+2000]=0; BITMAP[i+2250]=0; BITMAP[i+2500]=0; BITMAP[i+2750]=0; }
    for(i=0;i<250;++i) { BITMAP[i+3000]=0; BITMAP[i+3250]=0; BITMAP[i+3500]=0; BITMAP[i+3750]=0; }
    for(i=0;i<250;++i) { BITMAP[i+4000]=0; BITMAP[i+4250]=0; BITMAP[i+4500]=0; BITMAP[i+4750]=0; }
    for(i=0;i<250;++i) { BITMAP[i+5000]=0; BITMAP[i+5250]=0; BITMAP[i+5500]=0; BITMAP[i+5750]=0; }
    for(i=0;i<250;++i) { BITMAP[i+6000]=0; BITMAP[i+6250]=0; BITMAP[i+6500]=0; BITMAP[i+6750]=0; }
    for(i=0;i<250;++i) { BITMAP[i+7000]=0; BITMAP[i+7250]=0; BITMAP[i+7500]=0; BITMAP[i+7750]=0; }
    for(i=0;i<250;++i) { BMCOLOR[i]=0x70; BMCOLOR[i+250]=0x70; BMCOLOR[i+500]=0x70; BMCOLOR[i+750]=0x70; }

    /* Yellow C77 chassis: vents, metal identity rail, angular 77 marque. */
    bm_cells(0,0,40,6,0xff);
    for(i=0;i<5;++i) {
        bm_stroke(5,(int)(4+i*5),82,(int)(4+i*5),0);
        bm_stroke(232,(int)(4+i*5),308,(int)(4+i*5),0);
    }
    bm_color(11,1,18,4,LTGREY); bm_cells(11,1,18,4,0xff);
    bm_text(13,2,"COMMODORE",1); bm_text(13,3,"C77 // 64",1);
    /* Block Commodore C and the sharp twin-seven case emblem. */
    bm_stroke(14,9,8,14,0); bm_stroke(8,14,8,29,0); bm_stroke(8,29,14,34,0);
    bm_stroke(14,9,27,9,0); bm_stroke(14,34,27,34,0);
    bm_stroke(23,17,31,17,0); bm_stroke(23,25,31,25,0);
    bm_stroke(267,9,286,9,0); bm_stroke(286,9,273,34,0);
    bm_stroke(290,9,309,9,0); bm_stroke(309,9,296,34,0);
    bm_cyber_logo();
    /* Fixed wear map: chipped paint, grime blooms and case scratches. */
    bm_stroke(2,2,19,4,0); bm_line(35,30,58,27,0); bm_line(42,31,64,29,0);
    bm_line(71,14,79,11,0); bm_line(75,16,82,13,0); bm_dot(63,7,0); bm_dot(66,9,0);
    bm_line(193,7,218,10,0); bm_line(201,12,224,8,0); bm_dot(216,17,0); bm_dot(220,18,0);
    bm_line(313,3,318,9,0); bm_line(2,35,12,32,0); bm_dot(17,38,0); bm_dot(20,37,0);
    for(i=0;i<3;++i) bm_hline(0,(unsigned char)(40+i*3),40);

    /* Rear-equipment identity plate: fasteners, labels, warning and serial bars. */
    bm_cells(29,6,11,16,0xff); bm_color(29,6,11,16,LTGREY);
    bm_dot(236,51,0); bm_dot(315,51,0); bm_dot(236,171,0); bm_dot(315,171,0);
    bm_text(30,7,"C77 // 64",1);
    bm_text(30,8,"AGENDA UNIT",1);
    bm_text(30,9,"STAT",1); bm_color(35,9,1,1,dirty?LTRED:CYAN); bm_char(35,9,GL_NODE,0);
    bm_hline(1,47,27); bm_hline(1,171,27); bm_vline(8,48,124); bm_vline(224,48,124);
    draw_month_panel(); draw_day_feed();
    bm_line(242,157,248,146,0); bm_line(248,146,254,157,0); bm_line(254,157,242,157,0);
    bm_text(32,18,"5V DC",1); bm_text(30,19,"SN 077026",1);
    for(px=241;px<313;px+=4) bm_line((int)px,164,(int)px,(int)((px&4)?170:168),0);
    bm_line(238,73,249,70,0); bm_line(299,91,312,94,0); bm_dot(307,118,0); bm_dot(310,120,0);
    /* Perforated lower chassis with certification and command micro-labels. */
    bm_cells(0,22,40,3,0xff);
    for(px=8;px<168;px+=8) { bm_dot(px,181,0); bm_dot(px+3,185,0); }
    bm_text(1,22,"JOY/FIRE  A EVENT  S SAVE",1);
    bm_text(1,23,"D DELETE M MONTH T TODAY L DATA",1);
    bm_color(29,22,10,2,LTGREY); bm_cells(29,22,10,2,0xff);
    bm_text(30,22,"CBM C77",1); bm_text(30,23,"FCC // CE",1);
    bm_text(1,24,"Q EXIT // RED MARKER = EVENT",1);
}

static void modal(const char* title) {
    unsigned char i;
    char_mode();
    VIC_SPRITE_EN=0;
    fill(2,5,36,15,32,BLACK); box(2,5,36,15,YELLOW);
    fill(3,6,34,2,160,YELLOW); inv_text(4,6,title,YELLOW);
    for(i=4;i<36;i+=2) cell(i,19,(unsigned char)((i&2)?GL_HAZ1:GL_HAZ2),DARKGREY);
    cell(3,8,GL_NODE,CYAN); cell(36,8,GL_RIVET,YELLOW);
    screen_beep();
}

static void wait_key(void) { while(!kbhit()); cgetc(); }

static void title_screen(void) {
    unsigned char i; unsigned int x;
    bitmap_mode(); memset(BITMAP,0,8000); memset(BMCOLOR,0x70,1000);
    /* Night City terminal billboard replaces the earlier computer drawing. */
    bm_text(2,2,"C77 // PERSONAL DATA TERMINAL",0);
    bm_cyber_logo();
    for(i=0;i<5;++i) { bm_line(24,(int)(54+i*5),296,(int)(54+i*5),1); }
    bm_cells(4,9,32,7,0xff); bm_color(4,9,32,7,YELLOW);
    bm_cells(6,10,28,5,0); bm_color(6,10,28,5,YELLOW);
    bm_text(14,11,"NIGHT CITY",0);
    bm_text(9,13,"CYBERAGENDA TERMINAL",0);
    bm_stroke(35,72,52,88,1); bm_stroke(285,72,268,88,1);
    bm_color(5,17,30,2,PURPLE); bm_line(40,142,279,142,1); bm_line(56,151,264,151,1);
    for(x=48;x<280;x+=12) { bm_dot(x,137,1); bm_dot(x+4,158,1); }
    bm_text(8,19,"COMMODORE C77 // UNIT 64",0);
    bm_text(7,21,"BY EDUARDO A. DE CARVALHO",0);
    bm_text(7,23,"PRESS ANY KEY TO INITIALIZE",0);
    screen_beep(); wait_key();
}

static unsigned char input_field(unsigned char x,unsigned char y,char* out,unsigned char max) {
    unsigned char len=0,k,stored,display;
    fill(x,y,max,1,32,LTGREY);
    while(1) {
        cell((unsigned char)(x+len),y,GL_MARK,YELLOW);
        k=cgetc();
        if(k==0x0d || k==0x8d || k==0x0a || k==0x8a) { out[len]=0; return len; }
        if(k==27) { out[0]=0; return 0; }
        if((k==20 || k==8) && len) { --len; cell(x+len,y,32,LTGREY); }
        else {
            /* Use numeric values: cc65 target character literals are PETSCII. */
            display=0; stored=0;
            if(k>=0x41 && k<=0x5a) { display=(unsigned char)(k-0x40); stored=k; }
            else if(k>=0x61 && k<=0x7a) { display=(unsigned char)(k-0x60); stored=(unsigned char)(k-0x20); }
            else if(k>=0xc1 && k<=0xda) { display=(unsigned char)(k-0xc0); stored=(unsigned char)(k-0x80); }
            else if(k>=0x20 && k<=0x3f) { display=k; stored=k; }
            if(display && len<max) {
                out[len]=(char)stored;
                cell((unsigned char)(x+len),y,display,YELLOW);
                ++len;
            }
        }
    }
}

static unsigned char valid_digits(const char* s,unsigned char n) {
    unsigned char i; for(i=0;i<n;++i) if(s[i]<'0'||s[i]>'9') return 0; return s[n]==0;
}

static void add_event(void) {
    Event* e; char date[9];
    if(event_count>=MAX_EVENTS) { modal("DATABASE FULL"); text(5,11,"DELETE AN ENTRY FIRST",LTRED); wait_key(); return; }
    e=&events[event_count]; make_date(date,year,month,day);
    modal("NEW APPOINTMENT");
    text(5,9,"DATE",GREY); text(12,9,date,YELLOW); strcpy(e->date,date);
    text(5,11,"TIME",GREY); if(!input_field(12,11,e->time,4)) return;
    if(!valid_digits(e->time,4)) { text(12,13,"USE HHMM",LTRED); wait_key(); return; }
    text(5,14,"EVENT",GREY); if(!input_field(12,14,e->note,24)) return;
    ++event_count; dirty=1; sfx_ok();
}

static void delete_event(void) {
    unsigned char ix=first_event_on(day),i,k;
    modal("DELETE DAY ENTRY");
    if(ix==255) { text(5,11,"NO EVENT ON SELECTED DAY",GREY); wait_key(); return; }
    text(5,10,events[ix].time,CYAN); text(11,10,events[ix].note,YELLOW);
    text(5,13,"PRESS Y TO DELETE",LTRED); k=cgetc();
    if(k=='y'||k=='Y') { for(i=ix;i+1<event_count;++i) events[i]=events[i+1]; --event_count; dirty=1; }
}

static void list_events(void) {
    unsigned char i,start=0,k,row;
    do {
        modal("EVENT DATABASE"); row=9;
        if(!event_count) text(5,11,"NO EVENTS LOGGED",GREY);
        for(i=start;i<event_count && i<start+7;++i) {
            text(4,row,events[i].date,LTGREY); text(13,row,events[i].time,CYAN);
            text(19,row,events[i].note,YELLOW); ++row;
        }
        text(5,18,"SPACE NEXT // RETURN CLOSE",GREY); k=cgetc();
        if(k==' ' && start+7<event_count) start+=7; else break;
    } while(1);
}

static void set_today(void) {
    char in[9]; unsigned int y; unsigned char m,d;
    modal("SET SYSTEM DATE"); text(5,10,"YYYYMMDD",GREY);
    if(!input_field(15,10,in,8) || !valid_digits(in,8)) return;
    y=parse4(in); m=parse2(in+4); d=parse2(in+6);
    if(y<1900||y>2099||m<1||m>12||d<1||d>days_in_month(y,m)) { text(5,13,"INVALID DATE",LTRED); wait_key(); return; }
    strcpy(today,in); year=y; month=m; day=d; dirty=1; sfx_ok();
}

static void scratch_data(void) {
    cbm_open(15,8,15,"s:agenda.dat"); cbm_close(15);
}

static void save_data(void) {
    unsigned char i; unsigned int len;
    modal("WRITING DATA SHARD"); text(5,11,"DEVICE 8 // PLEASE WAIT",CYAN);
    scratch_data();
    if(cbm_open(2,8,2,"agenda.dat,s,w")!=0) { text(5,14,"DISK OPEN ERROR",LTRED); wait_key(); return; }
    cbm_write(2,today,8); cbm_write(2,"\r",1);
    { char n[4]; itoa(event_count,n,10); cbm_write(2,n,(unsigned int)strlen(n)); cbm_write(2,"\r",1); }
    for(i=0;i<event_count;++i) {
        len=(unsigned int)strlen(events[i].date); cbm_write(2,events[i].date,len); cbm_write(2,"\r",1);
        len=(unsigned int)strlen(events[i].time); cbm_write(2,events[i].time,len); cbm_write(2,"\r",1);
        len=(unsigned int)strlen(events[i].note); cbm_write(2,events[i].note,len); cbm_write(2,"\r",1);
    }
    cbm_close(2); dirty=0; text(5,14,"SAVE COMPLETE",LTGREEN); sfx_ok(); wait_key();
}

static unsigned char read_line(unsigned char lfn,char* out,unsigned char max) {
    unsigned char c,len=0; int got;
    while(len<max) { got=cbm_read(lfn,&c,1); if(got!=1) break; if(c==13||c==10) break; out[len++]=(char)c; }
    out[len]=0; return len;
}

static void load_data(void) {
    char n[4]; unsigned char i;
    event_count=0;
    if(cbm_open(2,8,2,"agenda.dat,s,r")!=0) return;
    if(read_line(2,today,8)!=8) strcpy(today,"20260831");
    read_line(2,n,3); event_count=(unsigned char)atoi(n); if(event_count>MAX_EVENTS) event_count=MAX_EVENTS;
    for(i=0;i<event_count;++i) {
        read_line(2,events[i].date,8); read_line(2,events[i].time,4); read_line(2,events[i].note,28);
    }
    cbm_close(2);
    year=parse4(today); month=parse2(today+4); day=parse2(today+6);
    if(year<1900||year>2099||month<1||month>12||day<1||day>days_in_month(year,month)) { year=2026;month=8;day=31; }
}

static void change_month(signed char delta) {
    if(delta<0) { if(month==1){month=12;--year;} else --month; }
    else { if(month==12){month=1;++year;} else ++month; }
    if(day>days_in_month(year,month)) day=days_in_month(year,month);
}

static void move_day(signed char delta) {
    signed int nd=(signed int)day+delta;
    if(nd<1) { change_month(-1); day=days_in_month(year,month); }
    else if(nd>days_in_month(year,month)) { change_month(1); day=1; }
    else day=(unsigned char)nd;
}

static unsigned char poll_input(void) {
    unsigned char j;
    if(kbhit()) return cgetc();
    j=JOY2;
    if(!(j&1)) return 145;
    if(!(j&2)) return 17;
    if(!(j&4)) return 157;
    if(!(j&8)) return 29;
    if(!(j&16)) return 13;
    return 0;
}

static void debounce(void) {
    unsigned int n; for(n=0;n<18000u;++n); while((JOY2&31)!=31);
}

int main(void) {
    unsigned char k,running=1;
    install_graphics();
    bordercolor(BLACK); bgcolor(BLACK); textcolor(YELLOW);
    load_data();
    title_screen();
    draw_calendar();
    while(running) {
        do { k=poll_input(); } while(!k);
        if(k==145) { move_selection(-7); sfx_move(); }
        else if(k==17) { move_selection(7); sfx_move(); }
        else if(k==157) { move_selection(-1); sfx_move(); }
        else if(k==29) { move_selection(1); sfx_move(); }
        else if(k==13) {
            unsigned char ix=first_event_on(day); const char* h=holiday_name(year,month,day);
            modal("SELECTED DAY");
            if(h) { text(5,9,"US HOLIDAY",GREY); text(16,9,h,YELLOW); }
            if(ix==255) text(5,h?13:11,"NO EVENTS // A TO ADD",GREY);
            else { text(5,h?13:10,events[ix].time,CYAN); text(11,h?13:10,events[ix].note,YELLOW); }
            k=cgetc();
            if(k=='a'||k=='A') add_event();
            refresh_dynamic();
        }
        else if(k=='a'||k=='A') { add_event(); refresh_dynamic(); }
        else if(k=='d'||k=='D') { delete_event(); refresh_dynamic(); }
        else if(k=='s'||k=='S') { save_data(); refresh_dynamic(); }
        else if(k=='t'||k=='T') { set_today(); refresh_dynamic(); }
        else if(k=='l'||k=='L') { list_events(); refresh_dynamic(); }
        else if(k=='m'||k=='M') { change_month(1); refresh_dynamic(); }
        else if(k=='q'||k=='Q') { if(dirty) save_data(); running=0; }
        debounce();
    }
    VIC_SPRITE_EN=0; *(unsigned char*)0xdd00=(unsigned char)((*(unsigned char*)0xdd00&0xfc)|3); *(unsigned char*)0xd018=0x14; clrscr();
    textcolor(YELLOW); cputs("C77//SESSION CLOSED\r\nSAFE TO UNMOUNT DISK IMAGE\r\n");
    return 0;
}
