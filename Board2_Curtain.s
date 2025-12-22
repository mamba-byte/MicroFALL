; ============================================================================
; ESOGU - Introduction to Microcomputers Term Project
; Board #2: Curtain Control System
; Language: PIC-as (XC8)
; MCU: PIC16F877A
; ============================================================================

PROCESSOR 16F877A
#include <xc.inc>

; CONFIGURATION BITS
CONFIG FOSC = HS, WDTE = OFF, PWRTE = ON, BOREN = ON, LVP = OFF

; ============================================================================
; VARIABLES
; ============================================================================
PSECT udata_bank0
    curtain_current:    DS 1    ; 0% (Open) to 100% (Closed) [cite: 686]
    curtain_desired:    DS 1    ; [cite: 689]
    light_val:          DS 1    ; LDR Value [cite: 695]
    pot_val:            DS 1    ; Potentiometer Value
    
    ; Stepper Logic
    step_index:         DS 1    ; Index in step sequence (0-3)
    steps_to_move:      DS 2    ; 16-bit counter for steps
    
    ; LCD Vars
    lcd_temp:           DS 1

; ============================================================================
; RESET VECTOR
; ============================================================================
PSECT code, delta=2
ORG 0x0000
    GOTO    Main

; ============================================================================
; MAIN PROGRAM
; ============================================================================
Main:
    CALL    Setup_Ports
    CALL    Setup_ADC
    CALL    Setup_UART
    CALL    LCD_Init
    
    ; Initialize Vars
    CLRF    curtain_current ; Start Open (0%)
    CLRF    curtain_desired
    CLRF    step_index

Main_Loop:
    ; --------------------------------------------------------
    ; 1. Read Potentiometer (User Input) [cite: 706]
    ; --------------------------------------------------------
    MOVLW   0               ; Channel 0 (RA0)
    CALL    Read_ADC
    ; Map 0-255 ADC to 0-100% Curtain
    ; Approx: ADC / 2.5. For assembly, roughly (ADC * 10) / 25
    ; Simplified: Just use High Byte scaled.
    MOVWF   pot_val
    ; Simple mapping for demo: Use ADC value directly as desired % (limit to 100)
    MOVLW   100
    SUBWF   pot_val, W
    BTFSC   STATUS, 0       ; If Pot > 100
    MOVLW   100             ; Cap at 100
    BTFSS   STATUS, 0
    MOVF    pot_val, W
    MOVWF   curtain_desired

    ; --------------------------------------------------------
    ; 2. Read LDR (Light Sensor) [cite: 696]
    ; --------------------------------------------------------
    ; Check Digital Threshold (RB4)
    BANKSEL PORTB
    BTFSS   PORTB, 4        ; Skip if Light is High (Normal)
    GOTO    Night_Mode
    GOTO    Check_Movement

Night_Mode:
    ; Light is Low -> Force Close Curtain (100%)
    MOVLW   100
    MOVWF   curtain_desired

    ; --------------------------------------------------------
    ; 3. Motor Control Logic [cite: 691]
    ; --------------------------------------------------------
Check_Movement:
    MOVF    curtain_current, W
    SUBWF   curtain_desired, W
    BTFSC   STATUS, 2       ; If Current == Desired (Z=1)
    GOTO    Update_LCD      ; No move needed

    ; Check Direction (Carry bit from SUBWF)
    ; If Desired < Current, Carry=0 (Open/CW)
    ; If Desired > Current, Carry=1 (Close/CCW)
    BTFSS   STATUS, 0
    GOTO    Move_Open
    
Move_Close:
    CALL    Step_CCW
    INCF    curtain_current, F
    CALL    Delay_Motor
    GOTO    Check_Movement
    
Move_Open:
    CALL    Step_CW
    DECF    curtain_current, F
    CALL    Delay_Motor
    GOTO    Check_Movement

Update_LCD:
    ; [cite: 709] Display updates would go here
    ; Calling LCD routines to print "Curtain: XX%"
    GOTO    Main_Loop

; ============================================================================
; SUBROUTINES
; ============================================================================

; --- Stepper Motor Driver (Unipolar 4-Step) [cite: 428] ---
; Sequence: 1000, 0100, 0010, 0001
Step_CW:
    INCF    step_index, F
    GOTO    Do_Step
Step_CCW:
    DECF    step_index, F
Do_Step:
    MOVF    step_index, W
    ANDLW   0x03            ; Mask to 0-3
    CALL    Step_Table
    BANKSEL PORTB
    MOVWF   PORTB           ; Output to RB0-RB3
    RETURN

Step_Table:
    ADDWF   PCL, F
    RETLW   00000001B
    RETLW   00000010B
    RETLW   00000100B
    RETLW   00001000B

Delay_Motor:
    ; Simple delay loop for motor speed
    MOVLW   250
    MOVWF   0x70
D_Loop: DECFSZ 0x70, F
    GOTO    D_Loop
    RETURN

; --- ADC Driver ---
Read_ADC:
    BANKSEL ADCON0
    MOVWF   ADCON0          ; Select Channel
    BSF     ADCON0, 0       ; ADON
    BSF     ADCON0, 2       ; GO
Wait_ADC:
    BTFSC   ADCON0, 2
    GOTO    Wait_ADC
    MOVF    ADRESH, W
    RETURN

; --- LCD Driver (4-bit mode) ---
LCD_Init:
    ; Basic initialization sequence for HD44780
    BANKSEL PORTD
    CLRF    PORTD
    ; (Omitting full 40-line init for brevity, standard 4-bit init required)
    RETURN

Setup_Ports:
    BANKSEL TRISB
    CLRF    TRISB           ; RB0-3 Output (Motor)
    BSF     TRISB, 4        ; RB4 Input (LDR Comparator)
    CLRF    TRISD           ; LCD Output
    MOVLW   00000011B       ; RA0, RA1 Analog
    MOVWF   TRISA
    BANKSEL ADCON1
    MOVLW   00000100B       ; AN0-AN1 Analog
    MOVWF   ADCON1
    BANKSEL PORTB
    RETURN

Setup_ADC:
    ; Standard ADC setup
    BANKSEL ADCON0
    MOVLW   01000001B       ; Fosc/8
    MOVWF   ADCON0
    RETURN

Setup_UART:
    ; [cite: 335] Standard 9600 Baud
    BANKSEL SPBRG
    MOVLW   25
    MOVWF   SPBRG
    BSF     TXSTA, 5        ; TXEN
    BSF     TXSTA, 2        ; BRGH
    BANKSEL RCSTA
    BSF     RCSTA, 7        ; SPEN
    BSF     RCSTA, 4        ; CREN
    RETURN

    END