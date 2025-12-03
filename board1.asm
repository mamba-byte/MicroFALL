; PROJE: CANLILIK TESTI (BLINK)
; Sadece Is?t?c? (RC2) yan?p sönecek.
#include <xc.inc>

; Sigorta Ayarlar? (Simülatör için en güvenlisi)
CONFIG FOSC = XT        ; 4MHz Kristal
CONFIG WDTE = OFF       ; Watchdog Kapal? (Çok önemli!)
CONFIG PWRTE = OFF
CONFIG BOREN = OFF
CONFIG LVP = OFF

PSECT code, abs
ORG 0x0000
    GOTO MAIN

MAIN:
    ; --- Bank 1 ---
    BANKSEL TRISC
    BCF     TRISC, 2    ; RC2 (Heater) ÇIKI? olsun
    
    ; --- Bank 0 ---
    BANKSEL PORTC
    
LOOP:
    ; LED YAK (Heater ON)
    BSF     PORTC, 2
    CALL    DELAY_SEC
    
    ; LED SÖNDÜR (Heater OFF)
    BCF     PORTC, 2
    CALL    DELAY_SEC
    
    GOTO    LOOP

; --- Gecikme Alt Program? ---
DELAY_SEC:
    MOVLW   255
    MOVWF   0x20
D1: MOVLW   255
    MOVWF   0x21
D2: DECFSZ  0x21, F
    GOTO    D2
    DECFSZ  0x20, F
    GOTO    D1
    RETURN

    END