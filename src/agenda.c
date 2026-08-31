#include <c64.h>
#include <cbm.h>
#include <conio.h>
#include <string.h>
#include <stdlib.h>

#define SCREEN ((unsigned char*)0x0400)
#define COLOR  ((unsigned char*)0xd800)
#define VIC_SPRITE_X (*(volatile unsigned char*)0xd000)
#define VIC_SPRITE_Y (*(volatile unsigned char*)0xd001)
#define VIC_SPRITE_EN (*(volatile unsigned char*)0xd015)
#define VIC_SPRITE_COL (*(volatile unsigned char*)0xd027)
#define VIC_SPRITE_PTR (*(volatile unsigned char*)0x07f8)
#define JOY2 (*(volatile unsigned char*)0xdc00)

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

static unsigned char screen_code(char c) {
    if (c >= 'a' && c <= 'z') c -= 32;
    if (c >= 'A' && c <= 'Z') return (unsigned char)(c - 'A' + 1);
    if (c >= '0' && c <= '9') return (unsigned char)c;
    if (c == '@') return 0;
    if (c == '-') return 45;
    if (c == '/') return 47;
    if (c == ':') return 58;
    if (c == '.') return 46;
    if (c == '+') return 43;
    if (c == '*') return 42;
    if (c == '>') return 62;
    if (c == '<') return 60;
    if (c == '#') return 35;
    if (c == '!') return 33;
    if (c == '?') return 63;
    return 32;
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

static void draw_header(void) {
    fill(0,0,40,25,32,BLACK);
    fill(0,0,40,3,160,YELLOW);
    inv_text(1,1,"C77",YELLOW);
    inv_text(10,1,"AGENDA // NIGHT CITY",YELLOW);
    inv_text(34,1,"V2.0",YELLOW);
    fill(0,3,40,1,GL_HZ,YELLOW);
}

static void selected_position(unsigned char* sx, unsigned char* sy) {
    unsigned char index=(unsigned char)(first_weekday(year,month)+day-1);
    *sx=(unsigned char)(2+(index%7)*4);
    *sy=(unsigned char)(8+(index/7)*2);
}

static void update_sprite(void) {
    unsigned char sx,sy;
    selected_position(&sx,&sy);
    VIC_SPRITE_X=(unsigned char)(24+sx*8);
    VIC_SPRITE_Y=(unsigned char)(50+sy*8);
    VIC_SPRITE_PTR=0x0d;
    VIC_SPRITE_COL=YELLOW;
    VIC_SPRITE_EN=1;
}

static void draw_sidebar(void) {
    unsigned char i, row=8, found=0; char date[9];
    box(30,5,10,16,DARKGREY);
    fill(31,6,8,1,160,YELLOW); inv_text(31,6,"DAY FEED",YELLOW);
    make_date(date,year,month,day);
    text(31,7,date,LTGREY);
    for(i=0;i<event_count && row<19;++i) {
        if(!strcmp(events[i].date,date)) {
            text(31,row,events[i].time,CYAN);
            text(31,row+1,events[i].note,YELLOW);
            row+=3; found=1;
        }
    }
    if(!found) { text(31,9,"NO EVENTS",GREY); text(31,10,"A TO ADD",DARKGREY); }
    text(31,19,"REC",GREY);
    { char n[3]; num2(n,event_count); text(35,19,n,YELLOW); }
}

static void draw_calendar(void) {
    static const char* wd[] = {"SU","MO","TU","WE","TH","FR","SA"};
    unsigned char i,d,idx,x,y,dim=days_in_month(year,month); char n[5];
    draw_header();
    box(0,5,30,16,YELLOW);
    text(2,6,"<",CYAN); text(5,6,months[month-1],YELLOW);
    num4(n,year); text(22,6,n,YELLOW); text(27,6,">",CYAN);
    for(i=0;i<7;++i) text((unsigned char)(2+i*4),7,wd[i],GREY);
    for(d=1;d<=dim;++d) {
        idx=(unsigned char)(first_weekday(year,month)+d-1);
        x=(unsigned char)(2+(idx%7)*4); y=(unsigned char)(8+(idx/7)*2);
        num2(n,d); text(x,y,n,d==day?BLACK:LTGREY);
        if(d==day) { cell(x-1,y,GL_MARK,YELLOW); cell(x,y,(unsigned char)(screen_code(n[0])+128),YELLOW); cell(x+1,y,(unsigned char)(screen_code(n[1])+128),YELLOW); }
        if(event_on(d)) cell(x+2,y,GL_MARK,CYAN);
    }
    draw_sidebar();
    fill(0,22,40,3,32,DARKGREY);
    text(1,22,"JOY MOVE  FIRE VIEW",LTGREY);
    text(1,23,"A ADD  D DELETE  S SAVE  T TODAY",YELLOW);
    text(1,24,"M MONTH  L LIST  Q EXIT",GREY);
    if(dirty) text(35,24,"MOD",LTRED);
    update_sprite();
}

static void modal(const char* title) {
    VIC_SPRITE_EN=0;
    fill(2,5,36,15,32,BLACK); box(2,5,36,15,YELLOW);
    fill(3,6,34,2,160,YELLOW); inv_text(4,6,title,YELLOW);
}

static void wait_key(void) { while(!kbhit()); cgetc(); }

static unsigned char input_field(unsigned char x,unsigned char y,char* out,unsigned char max) {
    unsigned char len=0,k,display;
    fill(x,y,max,1,32,LTGREY);
    while(1) {
        cell((unsigned char)(x+len),y,GL_MARK,YELLOW);
        k=cgetc();
        if(k==13) { out[len]=0; return len; }
        if(k==27) { out[0]=0; return 0; }
        if((k==20 || k==8) && len) { --len; cell(x+len,y,32,LTGREY); }
        else {
            /* C64 keyboards produce PETSCII $c1-$da for one letter case.
               Store normalized ASCII so display and disk data remain stable. */
            if(k>=0xc1 && k<=0xda) k=(unsigned char)(k-0x80);
            if(k>='a' && k<='z') k=(unsigned char)(k-32);
            if(k>='A' && k<='Z' && len<max) {
                /* PETSCII/ASCII A-Z is $41-$5a, but VIC screen A-Z is $01-$1a. */
                display=(unsigned char)(k-'A'+1);
                out[len]=(char)k;
                cell((unsigned char)(x+len),y,display,YELLOW);
                ++len;
            }
            else if(k>=32 && k<127 && len<max) {
                display=k;
                out[len]=(char)k;
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
    ++event_count; dirty=1;
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
    strcpy(today,in); year=y; month=m; day=d; dirty=1;
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
    cbm_close(2); dirty=0; text(5,14,"SAVE COMPLETE",LTGREEN); wait_key();
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
    while(running) {
        draw_calendar();
        do { k=poll_input(); } while(!k);
        if(k==145) move_day(-7);
        else if(k==17) move_day(7);
        else if(k==157) move_day(-1);
        else if(k==29) move_day(1);
        else if(k==13) { unsigned char ix=first_event_on(day); modal("SELECTED DAY"); if(ix==255) text(5,11,"NO EVENTS // A TO ADD",GREY); else { text(5,10,events[ix].time,CYAN);text(11,10,events[ix].note,YELLOW); } wait_key(); }
        else if(k=='a'||k=='A') add_event();
        else if(k=='d'||k=='D') delete_event();
        else if(k=='s'||k=='S') save_data();
        else if(k=='t'||k=='T') set_today();
        else if(k=='l'||k=='L') list_events();
        else if(k=='m'||k=='M') change_month(1);
        else if(k=='q'||k=='Q') { if(dirty) save_data(); running=0; }
        debounce();
    }
    VIC_SPRITE_EN=0; *(unsigned char*)0xd018=0x14; clrscr();
    textcolor(YELLOW); cputs("C77//SESSION CLOSED\r\nSAFE TO UNMOUNT DISK IMAGE\r\n");
    return 0;
}
