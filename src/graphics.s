.export _install_graphics

.segment "CODE"

_install_graphics:
        sei
        lda $01
        pha
        and #$fb
        sta $01
        ldx #$00
copyrom:
        lda $d000,x
        sta $3800,x
        lda $d100,x
        sta $3900,x
        lda $d200,x
        sta $3a00,x
        lda $d300,x
        sta $3b00,x
        lda $d400,x
        sta $3c00,x
        lda $d500,x
        sta $3d00,x
        lda $d600,x
        sta $3e00,x
        lda $d700,x
        sta $3f00,x
        inx
        bne copyrom
        pla
        sta $01

        ldx #$00
custom:
        lda glyphs,x
        sta $38d8,x
        inx
        cpx #40
        bne custom
        ldx #$00
custom2:
        lda glyphs2,x
        sta $3af0,x
        inx
        cpx #16
        bne custom2

        ldx #$00
sprite:
        lda cursor_sprite,x
        sta $0340,x
        inx
        cpx #64
        bne sprite

        lda #$1e
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

cursor_sprite:
        .byte $ff,$ff,$ff,$80,$00,$01,$80,$00,$01
        .byte $80,$00,$01,$80,$00,$01,$80,$00,$01
        .byte $80,$00,$01,$80,$00,$01,$80,$00,$01
        .byte $80,$00,$01,$80,$00,$01,$80,$00,$01
        .byte $80,$00,$01,$80,$00,$01,$80,$00,$01
        .byte $80,$00,$01,$80,$00,$01,$80,$00,$01
        .byte $80,$00,$01,$ff,$ff,$ff,$00,$00,$00,$00
