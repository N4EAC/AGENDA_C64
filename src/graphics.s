.export _install_graphics

.segment "CODE"

_install_graphics:
        sei
        lda $dd00
        and #$fc
        ora #$01
        sta $dd00
        lda $01
        pha
        and #$fb
        sta $01
        ldx #$00
copyrom:
        lda $d000,x
        sta $9800,x
        lda $d100,x
        sta $9900,x
        lda $d200,x
        sta $9a00,x
        lda $d300,x
        sta $9b00,x
        lda $d400,x
        sta $9c00,x
        lda $d500,x
        sta $9d00,x
        lda $d600,x
        sta $9e00,x
        lda $d700,x
        sta $9f00,x
        inx
        bne copyrom
        pla
        sta $01

        ldx #$00
custom:
        lda glyphs,x
        sta $98d8,x
        inx
        cpx #40
        bne custom
        ldx #$00
custom2:
        lda glyphs2,x
        sta $9af0,x
        inx
        cpx #16
        bne custom2
        ldx #$00
custom3:
        lda caseglyphs,x
        sta $9b00,x
        inx
        cpx #64
        bne custom3

        ldx #$00
sprite:
        lda cursor_sprite,x
        sta $8b40,x
        inx
        cpx #64
        bne sprite

        lda #$36
        sta $d018
        cli
        rts

glyphs:
        .byte $ff,$c0,$c0,$c0,$c0,$c0,$c0,$c0
        .byte $ff,$03,$03,$03,$03,$03,$03,$03
        .byte $c0,$c0,$c0,$c0,$c0,$c0,$c0,$ff
        .byte $03,$03,$03,$03,$03,$03,$03,$ff
        .byte $00,$00,$00,$ff,$ff,$00,$00,$00
glyphs2:
        .byte $18,$18,$18,$18,$18,$18,$18,$18
        .byte $aa,$55,$aa,$55,$aa,$55,$aa,$55
caseglyphs:
        .byte $c0,$60,$30,$18,$0c,$06,$03,$01
        .byte $03,$06,$0c,$18,$30,$60,$c0,$80
        .byte $18,$3c,$7e,$ff,$7e,$3c,$18,$00
        .byte $00,$00,$00,$ff,$18,$18,$18,$18
        .byte $aa,$00,$aa,$00,$aa,$00,$aa,$00
        .byte $00,$18,$18,$7e,$7e,$18,$18,$00
        .byte $80,$c0,$e0,$f0,$e0,$c0,$80,$00
        .byte $81,$c3,$e7,$ff,$e7,$c3,$81,$00

cursor_sprite:
        .byte $ff,$ff,$ff,$80,$00,$01,$80,$00,$01
        .byte $80,$00,$01,$80,$00,$01,$80,$00,$01
        .byte $80,$00,$01,$80,$00,$01,$80,$00,$01
        .byte $80,$00,$01,$80,$00,$01,$80,$00,$01
        .byte $80,$00,$01,$80,$00,$01,$80,$00,$01
        .byte $80,$00,$01,$80,$00,$01,$80,$00,$01
        .byte $80,$00,$01,$ff,$ff,$ff,$00,$00,$00,$00
