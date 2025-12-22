; ============================================================================
; ESOGU - Introduction to Microcomputers Term Project
; Board #1: Air Conditioner System
; Language: PIC-as (XC8)
; MCU: PIC16F877A
; ============================================================================

PROCESSOR 16F877A
#include <xc.inc>

; ============================================================================
; CONFIGURATION BITS
; ============================================================================
CONFIG FOSC = HS        ; High Speed Oscillator
CONFIG WDTE = OFF       ; Watchdog Timer Disabled
CONFIG PWRTE = ON       ; Power-up Timer Enabled
CONFIG BOREN = ON       ; Brown-out Reset Enabled
CONFIG LVP = OFF        ; Low Voltage Programming Disabled
CONFIG CPD = OFF        ; Data EEPROM Memory Code Protection Off
CONFIG WRT = OFF        ; Flash Program Memory Write Enable Off
CONFIG CP = OFF         ; Flash Program Memory Code Protection Off

; ============================================================================
; VARIABLES (Data Segment)
; ============================================================================
PSECT udata_bank0
    
    ; System State Variables
    current_temp:       DS 1    ; Current Ambient Temp (Raw ADC high byte)
    desired_temp:       DS 1    ; Desired Temp (Set by user)
    fan_speed:          DS 1    ; Fan speed (dummy placeholder for logic)
    
    ; Display Multiplexing Variables
    digit_1:            DS 1    ; Leftmost digit data
    digit_2:            DS 1
    digit_3:            DS 1
    digit_4:            DS 1    ; Rightmost digit data
    digit_counter:      DS 1    ; Tracks which digit is currently active
    
    ; Keypad Variables
    key_pressed:        DS 1
    temp_key:           DS 1
    
    ; General Purpose
    w_temp:             DS 1    ; Context saving for ISR
    status_temp:        DS 1

; ============================================================================
; RESET VECTOR
; ============================================================================
PSECT code, delta=2
ORG 0x0000
    GOTO    Main

; ============================================================================
; INTERRUPT VECTOR
; ============================================================================
ORG 0x0004
ISR:
    ; Context Save
    MOVWF   w_temp
    SWAPF   STATUS, W
    MOVWF   status_temp

    ; Check Timer0 Interrupt (For 7-Segment Multiplexing)
    BANKSEL INTCON
    BTFSS   INTCON, 2       ; Check T0IF
    GOTO    Check_UART

    CALL    Refresh_Display
    BCF     INTCON, 2       ; Clear T0IF

Check_UART:
    ; UART Handling (Placeholder for R2.1.4-1)
    ; Check PIR1, RCIF here for serial commands
    
Exit_ISR:
    ; Context Restore
    SWAPF   status_temp, W
    MOVWF   STATUS
    SWAPF   w_temp, F
    SWAPF   w_temp, W
    RETFIE

; ============================================================================
; MAIN PROGRAM
; ============================================================================
Main:
    ; 1. Initialization
    CALL    Setup_Ports
    CALL    Setup_ADC
    CALL    Setup_Timer0
    CALL    Setup_UART

    ; Initialize Variables
    MOVLW   25              ; Default desired temp = 25 degrees (approx)
    MOVWF   desired_temp
    CLRF    digit_counter

Main_Loop:
    ; ---------------------------------------------------------
    ; Task 1: Read Ambient Temperature [R2.1.1-4]
    ; ---------------------------------------------------------
    CALL    Read_ADC_Temp
    MOVWF   current_temp

    ; ---------------------------------------------------------
    ; Task 2: Temperature Control Logic [R2.1.1-2, R2.1.1-3]
    ; ---------------------------------------------------------
    ; Compare Desired vs Current
    MOVF    current_temp, W
    SUBWF   desired_temp, W ; W = Desired - Current
    
    ; Check Status bits to see result
    BTFSS   STATUS, 0       ; Check Carry (C=0 if Result is Negative -> Current > Desired)
    GOTO    Cooling_Mode    ; Current is hotter than desired

Heating_Mode:
    ; Desired > Current -> Turn Heater ON, Cooler OFF
    BANKSEL PORTC
    BSF     PORTC, 0        ; Heater ON
    BCF     PORTC, 1        ; Cooler OFF
    GOTO    Update_Display_Vars

Cooling_Mode:
    ; Current > Desired -> Turn Heater OFF, Cooler ON
    BANKSEL PORTC
    BCF     PORTC, 0        ; Heater OFF
    BSF     PORTC, 1        ; Cooler ON

    ; ---------------------------------------------------------
    ; Task 3: Update Display Data [R2.1.3-1]
    ; ---------------------------------------------------------
Update_Display_Vars:
    ; For this project, we display Ambient Temp on digits 3&4
    ; and Desired Temp on digits 1&2.
    
    ; Convert 'desired_temp' to BCD for display
    MOVF    desired_temp, W
    CALL    Hex_To_7Seg     ; Simplified: just showing raw hex/lookup for now
    MOVWF   digit_1         
    
    MOVF    current_temp, W
    CALL    Hex_To_7Seg
    MOVWF   digit_3

    ; ---------------------------------------------------------
    ; Task 4: Scan Keypad [R2.1.2]
    ; ---------------------------------------------------------
    CALL    Keypad_Scan
    MOVF    key_pressed, W
    BTFSS   STATUS, 2       ; If Z=1 (Result 0), no key pressed
    MOVWF   desired_temp    ; SIMPLE LOGIC: If key pressed, set as desired temp (demo)

    GOTO    Main_Loop

; ============================================================================
; SUBROUTINES
; ============================================================================

; --- Setup Ports ---
Setup_Ports:
    BANKSEL TRISA
    ; RA0 = Input (ADC), RA2-RA5 = Output (Display Commons)
    MOVLW   00000001B       
    MOVWF   TRISA
    
    ; PORTB = Keypad (Rows Out, Cols In or Vice Versa)
    ; Assuming RB0-3 Output (Rows), RB4-7 Input (Cols)
    MOVLW   11110000B
    MOVWF   TRISB
    
    ; PORTC = Outputs (Heater/Cooler) + UART
    MOVLW   10000000B       ; RC7 RX Input, others Output
    MOVWF   TRISC
    
    ; PORTD = Output (7-Segment Segments)
    CLRF    TRISD
    
    BANKSEL ADCON1
    MOVLW   00001110B       ; AN0 Analog, others Digital, Left Justified
    MOVWF   ADCON1
    
    BANKSEL PORTA
    CLRF    PORTA
    CLRF    PORTC
    CLRF    PORTD
    RETURN

; --- Setup ADC ---
Setup_ADC:
    BANKSEL ADCON0
    ; Fosc/32, Channel 0 (RA0), ADC On
    MOVLW   10000001B       ; ADCS1:0=10, CHS=000, ADON=1
    MOVWF   ADCON0
    RETURN

; --- Read ADC (Returns High Byte in W) ---
Read_ADC_Temp:
    BANKSEL ADCON0
    BSF     ADCON0, 2       ; Start Conversion (GO/DONE)
Wait_ADC:
    BTFSC   ADCON0, 2       ; Wait for GO/DONE to clear
    GOTO    Wait_ADC
    
    MOVF    ADRESH, W       ; Read high byte (8-bit resolution mode)
    RETURN

; --- Setup Timer0 (For Display Multiplexing) ---
Setup_Timer0:
    BANKSEL OPTION_REG
    ; Prescaler 1:64 assigned to TMR0
    MOVLW   11010101B       ; T0CS=0, PSA=0, PS=101
    MOVWF   OPTION_REG
    
    BANKSEL INTCON
    BSF     INTCON, 5       ; Enable T0IE
    BSF     INTCON, 7       ; Enable GIE
    RETURN

; --- Setup UART ---
Setup_UART:
    BANKSEL SPBRG
    MOVLW   25              ; 9600 Baud at 4MHz (Adjust for your crystal)
    MOVWF   SPBRG
    
    BSF     TXSTA, 2        ; BRGH High Speed
    BCF     TXSTA, 4        ; Sync = 0 (Async)
    BSF     TXSTA, 5        ; TXEN = 1
    
    BANKSEL RCSTA
    BSF     RCSTA, 7        ; SPEN = 1
    BSF     RCSTA, 4        ; CREN = 1
    RETURN

; --- Refresh Display (Called from ISR) ---
Refresh_Display:
    ; 1. Turn off all digits (Active Low or High depending on hardware)
    ; Assuming Common Cathode (Active Low to turn on common? Check PDF)
    ; PDF Fig 5 shows transistors driving commons. Usually Logic 1 at Base turns it ON.
    BANKSEL PORTA
    BCF     PORTA, 2
    BCF     PORTA, 3
    BCF     PORTA, 4
    BCF     PORTA, 5
    
    ; 2. Determine which digit to light up
    MOVF    digit_counter, W
    ANDLW   0x03            ; Mask to 0-3
    MOVWF   digit_counter
    
    ; Jump table or Branching
    MOVF    digit_counter, W
    BTFSC   STATUS, 2       ; If 0
    GOTO    Show_Digit_1
    XORLW   1
    BTFSC   STATUS, 2       ; If 1
    GOTO    Show_Digit_2
    XORLW   3               ; (1^2) = 3... simple compare better
    DECFSZ  digit_counter, W
    GOTO    Show_Digit_3
    GOTO    Show_Digit_4

Show_Digit_1:
    MOVF    digit_1, W
    MOVWF   PORTD
    BSF     PORTA, 2        ; Turn on Digit 1 Common
    GOTO    End_Refresh

Show_Digit_2:
    MOVF    digit_2, W
    MOVWF   PORTD
    BSF     PORTA, 3
    GOTO    End_Refresh

Show_Digit_3:
    MOVF    digit_3, W
    MOVWF   PORTD
    BSF     PORTA, 4
    GOTO    End_Refresh

Show_Digit_4:
    MOVF    digit_4, W
    MOVWF   PORTD
    BSF     PORTA, 5
    GOTO    End_Refresh

End_Refresh:
    INCF    digit_counter, F
    RETURN

; --- Keypad Scan (Simple Polling) ---
Keypad_Scan:
    CLRF    key_pressed
    BANKSEL PORTB
    ; Set Row 1 Low, others High
    MOVLW   11111110B
    MOVWF   PORTB
    NOP
    BTFSS   PORTB, 4        ; Check Col 1
    MOVLW   1               ; Key 1 pressed
    ; ... (Add full matrix scan logic here) ...
    RETURN

; --- 7-Segment Look Up Table (Hex to 7-Seg Pattern) ---
Hex_To_7Seg:
    ANDLW   0x0F
    ADDWF   PCL, F
    RETLW   0x3F ; 0
    RETLW   0x06 ; 1
    RETLW   0x5B ; 2
    RETLW   0x4F ; 3
    RETLW   0x66 ; 4
    RETLW   0x6D ; 5
    RETLW   0x7D ; 6
    RETLW   0x07 ; 7
    RETLW   0x7F ; 8
    RETLW   0x6F ; 9
    RETLW   0x77 ; A
    RETLW   0x7C ; B
    RETLW   0x39 ; C
    RETLW   0x5E ; D
    RETLW   0x79 ; E
    RETLW   0x71 ; F

    END