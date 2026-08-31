#!/usr/bin/env python3
"""Build C77 Agenda as a tokenized C64 BASIC V2 PRG and writable D64 image."""

from pathlib import Path
import struct

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "outputs"

# Commodore BASIC V2 tokens.
TOKENS = {
    "END": 0x80, "FOR": 0x81, "NEXT": 0x82, "DATA": 0x83, "INPUT#": 0x84,
    "INPUT": 0x85, "DIM": 0x86, "READ": 0x87, "LET": 0x88, "GOTO": 0x89,
    "RUN": 0x8A, "IF": 0x8B, "RESTORE": 0x8C, "GOSUB": 0x8D, "RETURN": 0x8E,
    "REM": 0x8F, "STOP": 0x90, "ON": 0x91, "WAIT": 0x92, "LOAD": 0x93,
    "SAVE": 0x94, "VERIFY": 0x95, "DEF": 0x96, "POKE": 0x97, "PRINT#": 0x98,
    "PRINT": 0x99, "CONT": 0x9A, "LIST": 0x9B, "CLR": 0x9C, "CMD": 0x9D,
    "SYS": 0x9E, "OPEN": 0x9F, "CLOSE": 0xA0, "GET": 0xA1, "NEW": 0xA2,
    "TAB(": 0xA3, "TO": 0xA4, "FN": 0xA5, "SPC(": 0xA6, "THEN": 0xA7,
    "NOT": 0xA8, "STEP": 0xA9, "+": 0xAA, "-": 0xAB, "*": 0xAC, "/": 0xAD,
    "^": 0xAE, "AND": 0xAF, "OR": 0xB0, ">": 0xB1, "=": 0xB2, "<": 0xB3,
    "SGN": 0xB4, "INT": 0xB5, "ABS": 0xB6, "USR": 0xB7, "FRE": 0xB8,
    "POS": 0xB9, "SQR": 0xBA, "RND": 0xBB, "LOG": 0xBC, "EXP": 0xBD,
    "COS": 0xBE, "SIN": 0xBF, "TAN": 0xC0, "ATN": 0xC1, "PEEK": 0xC2,
    "LEN": 0xC3, "STR$": 0xC4, "VAL": 0xC5, "ASC": 0xC6, "CHR$": 0xC7,
    "LEFT$": 0xC8, "RIGHT$": 0xC9, "MID$": 0xCA, "GO": 0xCB,
}

# Source stays intentionally plain ASCII; tokenization converts unquoted text to PETSCII.
SOURCE = r'''
10 POKE53280,7:POKE53281,0:POKE646,7:PRINTCHR$(147):DIMD$(60),T$(60),N$(60):GOSUB9000:GOSUB8000
20 GOSUB1000
30 GETK$:IFK$=""THEN30
40 IFK$="1"THENGOSUB2000
50 IFK$="2"THENGOSUB3000
60 IFK$="3"THENGOSUB4000
70 IFK$="4"THENGOSUB5000
80 IFK$="5"THENGOSUB6000
90 IFK$="6"THENGOSUB7000
100 IFK$="7"THENGOSUB8500
110 IFK$="8"THENGOSUB8800
120 GOTO20
1000 PRINTCHR$(147):POKE646,7:PRINT"C77//AGENDA  NIGHT CITY ORGANIZER":POKE646,12:PRINT"----------------------------------------"
1010 POKE646,15:PRINT"TODAY ";TD$;"   RECORDS ";NR
1020 POKE646,12:PRINT:PRINT"[1] CALENDAR     [2] ADD EVENT":PRINT"[3] DAY VIEW     [4] ALL EVENTS":PRINT"[5] DELETE       [6] SET TODAY":PRINT"[7] SAVE         [8] SAVE + EXIT"
1030 POKE646,10:PRINT:PRINT"SELECT MODULE > ";:RETURN
2000 PRINTCHR$(147):POKE646,7:PRINT"C77//MONTH GRID":POKE646,15:INPUT"MONTH (1-12)";MM:INPUT"YEAR (1900-2099)";YY
2010 IFMM<1ORMM>12ORYY<1900ORYY>2099THENRETURN
2020 M=MM:Y=YY:IFM<3THENM=M+12:Y=Y-1
2030 W=(1+INT(13*(M+1)/5)+Y+INT(Y/4)-INT(Y/100)+INT(Y/400))-(INT((1+INT(13*(M+1)/5)+Y+INT(Y/4)-INT(Y/100)+INT(Y/400))/7)*7):W=W+6:W=W-INT(W/7)*7
2040 GOSUB9200:PRINTCHR$(147):POKE646,7:PRINT"C77//";MN$;" ";YY:POKE646,12:PRINT"SU  MO  TU  WE  TH  FR  SA"
2050 FORI=1TOW:PRINT"    ";:NEXT
2060 FORDD=1TODM:GOSUB9400:A$=" ":FORJ=1TONR:IFLEFT$(D$(J),8)=Q$THENA$="*"
2070 NEXTJ:K$=RIGHT$(" "+MID$(STR$(DD),2),2):POKE646,15:PRINTK$;:POKE646,10:PRINTA$;" ";:W=W+1:IFW=7THENPRINT:W=0
2080 NEXTDD:PRINT:POKE646,10:PRINT"* EVENT LOGGED":GOSUB9900:RETURN
3000 IFNR=60THENPRINT"DATABASE FULL":GOSUB9900:RETURN
3010 PRINTCHR$(147):POKE646,7:PRINT"C77//NEW ENTRY":POKE646,15:INPUT"DATE YYYYMMDD";A$:IFA$=""THENRETURN
3020 IFLEN(A$)<>8THENPRINT"INVALID DATE FORMAT":GOSUB9900:RETURN
3030 INPUT"TIME HHMM";B$:IFLEN(B$)<>4THENPRINT"INVALID TIME FORMAT":GOSUB9900:RETURN
3040 INPUT"EVENT (MAX 28 CHARS)";C$:IFLEN(C$)>28THENC$=LEFT$(C$,28)
3050 NR=NR+1:D$(NR)=A$:T$(NR)=B$:N$(NR)=C$:DS=1:POKE646,10:PRINT"ENTRY BUFFERED - USE SAVE":GOSUB9900:RETURN
4000 PRINTCHR$(147):POKE646,7:PRINT"C77//DAY SCAN":POKE646,15:INPUT"DATE YYYYMMDD";A$:F=0
4010 FORI=1TONR:IFD$(I)=A$THENPOKE646,10:PRINTT$(I);"  ";N$(I):F=1
4020 NEXT:POKE646,12:IFF=0THENPRINT"NO EVENTS DETECTED"
4030 GOSUB9900:RETURN
5000 PRINTCHR$(147):POKE646,7:PRINT"C77//EVENT DATABASE":POKE646,15:I=1
5010 IFNR=0THENPRINT"NO EVENTS DETECTED":GOSUB9900:RETURN
5020 PRINTI;" ";D$(I);" ";T$(I):POKE646,10:PRINT"  ";N$(I):I=I+1
5030 IFI<=NRANDI/7<>INT(I/7)THEN5020
5040 IFI<=NRTHENGOSUB9900:PRINTCHR$(147):GOTO5020
5050 GOSUB9900:RETURN
6000 PRINTCHR$(147):POKE646,7:PRINT"C77//DELETE ENTRY":POKE646,15:IFNR=0THENPRINT"DATABASE EMPTY":GOSUB9900:RETURN
6010 FORI=1TONR:PRINTI;" ";D$(I);" ";T$(I);" ";LEFT$(N$(I),15):NEXT
6020 INPUT"ENTRY NUMBER (0 CANCEL)";X:IFX=0THENRETURN
6030 IFX<1ORX>NRTHENRETURN
6040 FORI=XTONR-1:D$(I)=D$(I+1):T$(I)=T$(I+1):N$(I)=N$(I+1):NEXT:NR=NR-1:DS=1:PRINT"ENTRY DELETED":GOSUB9900:RETURN
7000 PRINTCHR$(147):POKE646,7:PRINT"C77//SYSTEM DATE":POKE646,15:INPUT"TODAY YYYYMMDD";A$:IFLEN(A$)<>8THENRETURN
7010 TD$=A$:DS=1:PRINT"DATE UPDATED":GOSUB9900:RETURN
8000 NR=0:DS=0:OPEN2,8,2,"AGENDA.DAT,S,R":INPUT#2,TD$,NR:IFNR>60THENNR=60
8010 FORI=1TONR:INPUT#2,D$(I),T$(I),N$(I):NEXT:CLOSE2:RETURN
8500 POKE646,10:PRINTCHR$(147):PRINT"C77//WRITING DATA SHARD...":OPEN15,8,15,"S:AGENDA.DAT":CLOSE15
8510 OPEN2,8,2,"AGENDA.DAT,S,W":PRINT#2,TD$:PRINT#2,NR:FORI=1TONR:PRINT#2,D$(I):PRINT#2,T$(I):PRINT#2,N$(I):NEXT:CLOSE2:DS=0
8520 PRINT"SAVE COMPLETE":GOSUB9900:RETURN
8800 IFDS=1THENGOSUB8500
8810 PRINTCHR$(147):POKE646,7:PRINT"C77//SESSION CLOSED":POKE646,12:PRINT"SAFE TO UNMOUNT DISK IMAGE":END
9000 TD$="20260831":RETURN
9200 DM=31:MN$="JANUARY":IFMM=2THENDM=28:MN$="FEBRUARY"
9210 IFMM=3THENMN$="MARCH"
9220 IFMM=4THENDM=30:MN$="APRIL"
9230 IFMM=5THENMN$="MAY"
9240 IFMM=6THENDM=30:MN$="JUNE"
9250 IFMM=7THENMN$="JULY"
9260 IFMM=8THENMN$="AUGUST"
9270 IFMM=9THENDM=30:MN$="SEPTEMBER"
9280 IFMM=10THENMN$="OCTOBER"
9290 IFMM=11THENDM=30:MN$="NOVEMBER"
9300 IFMM=12THENMN$="DECEMBER"
9310 IFMM=2AND(YY-INT(YY/4)*4=0)THENDM=29
9320 IFMM=2AND(YY-INT(YY/100)*100=0)AND(YY-INT(YY/400)*400<>0)THENDM=28
9330 RETURN
9400 Q$=MID$(STR$(YY),2)+RIGHT$("0"+MID$(STR$(MM),2),2)+RIGHT$("0"+MID$(STR$(DD),2),2):RETURN
9900 POKE646,12:PRINT:PRINT"PRESS ANY KEY";
9910 GETZ$:IFZ$=""THEN9910
9920 RETURN
'''


def tokenize_line(text: str) -> bytes:
    out = bytearray()
    i = 0
    quoted = False
    rem = False
    keys = sorted(TOKENS, key=len, reverse=True)
    while i < len(text):
        ch = text[i]
        if ch == '"':
            quoted = not quoted
            out.append(ord(ch)); i += 1; continue
        if not quoted and not rem:
            matched = None
            for key in keys:
                if text.startswith(key, i):
                    matched = key; break
            if matched:
                out.append(TOKENS[matched]); i += len(matched)
                if matched == "REM": rem = True
                continue
        out.append(ord(ch.upper()) if 'a' <= ch <= 'z' and not quoted else ord(ch))
        i += 1
    return bytes(out)


def make_prg(source: str) -> bytes:
    lines = []
    for raw in source.strip().splitlines():
        number, text = raw.strip().split(' ', 1)
        lines.append((int(number), tokenize_line(text)))
    addr = 0x0801
    body = bytearray()
    for number, code in lines:
        next_addr = addr + 2 + 2 + len(code) + 1
        body += struct.pack('<HH', next_addr, number) + code + b'\x00'
        addr = next_addr
    body += b'\x00\x00'
    return b'\x01\x08' + body


# Standard 35-track 1541 geometry.
SECTORS = [0] + [21]*17 + [19]*7 + [18]*6 + [17]*5

def ts_offset(track: int, sector: int) -> int:
    return (sum(SECTORS[1:track]) + sector) * 256

def petscii_name(name: str, length=16) -> bytes:
    return name.upper().encode('ascii')[:length].ljust(length, b'\xA0')

def make_d64(files):
    img = bytearray(174848)
    free = {(t, s) for t in range(1, 36) for s in range(SECTORS[t])}
    free.remove((18, 0)); free.remove((18, 1))
    # BAM and disk header.
    bam = ts_offset(18, 0)
    img[bam:bam+4] = bytes([18, 1, 0x41, 0])
    img[bam+0x90:bam+0xA0] = petscii_name("C77 AGENDA")
    img[bam+0xA2:bam+0xA4] = b'77'
    img[bam+0xA5:bam+0xA7] = b'2A'
    # Directory sector ends chain.
    directory = ts_offset(18, 1)
    img[directory:directory+2] = b'\x00\xFF'
    entries = []
    for name, data, ftype in files:
        chain = []
        remaining = data
        while remaining:
            # Prefer low tracks and keep directory track untouched.
            t, s = min(free)
            free.remove((t, s)); chain.append((t, s))
            chunk, remaining = remaining[:254], remaining[254:]
            off = ts_offset(t, s)
            if remaining:
                img[off:off+2] = bytes(min(free)) if False else b'\x00\x00'
            img[off+2:off+2+len(chunk)] = chunk
        for idx, (t, s) in enumerate(chain):
            off = ts_offset(t, s)
            if idx + 1 < len(chain): img[off:off+2] = bytes(chain[idx+1])
            else: img[off:off+2] = bytes([0, len(data[-254:]) + 1])
        entries.append((name, chain[0], len(chain), ftype))
    # Directory entries (max 8 here).
    for idx, (name, (t, s), blocks, ftype) in enumerate(entries):
        off = directory + 2 + idx*32
        img[off] = 0x80 | ftype
        img[off+1:off+3] = bytes([t, s])
        img[off+3:off+19] = petscii_name(name)
        img[off+28:off+30] = struct.pack('<H', blocks)
    # BAM free counts and maps.
    for t in range(1, 36):
        available = [s for s in range(SECTORS[t]) if (t, s) in free]
        off = bam + 4*t
        img[off] = len(available)
        mask = 0
        for s in available: mask |= 1 << s
        img[off+1:off+4] = mask.to_bytes(3, 'little')
    return bytes(img)


def main():
    OUT.mkdir(exist_ok=True)
    prg = make_prg(SOURCE)
    # Seed file prevents BASIC's OPEN-for-read from raising FILE NOT FOUND.
    seed = b'20260831\r0\r'
    d64 = make_d64([("C77 AGENDA", prg, 2), ("AGENDA.DAT", seed, 1)])
    (OUT / "C77_AGENDA.d64").write_bytes(d64)
    (OUT / "C77_AGENDA.prg").write_bytes(prg)
    (OUT / "C77_AGENDA_source.bas").write_text(SOURCE.strip()+"\n", encoding="ascii")
    print(f"PRG: {len(prg)} bytes; D64: {len(d64)} bytes")

if __name__ == "__main__":
    main()
