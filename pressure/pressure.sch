EESchema Schematic File Version 2  date 11/17/2012 10:44:11 AM
LIBS:power
LIBS:kirill
LIBS:device
LIBS:transistors
LIBS:conn
LIBS:linear
LIBS:regul
LIBS:74xx
LIBS:cmos4000
LIBS:adc-dac
LIBS:memory
LIBS:xilinx
LIBS:special
LIBS:microcontrollers
LIBS:dsp
LIBS:microchip
LIBS:analog_switches
LIBS:motorola
LIBS:texas
LIBS:intel
LIBS:audio
LIBS:interface
LIBS:digital-audio
LIBS:philips
LIBS:display
LIBS:cypress
LIBS:siliconi
LIBS:opto
LIBS:atmel
LIBS:contrib
LIBS:valves
LIBS:pressure-cache
EELAYER 25  0
EELAYER END
$Descr A4 11700 8267
encoding utf-8
Sheet 1 1
Title ""
Date "17 nov 2012"
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
Connection ~ 8400 4100
Wire Wire Line
	8650 4100 8400 4100
Connection ~ 8400 4700
Wire Wire Line
	8400 4500 8400 4700
Wire Wire Line
	8650 4300 8650 4450
Wire Wire Line
	8650 4450 9650 4450
Wire Wire Line
	9650 4450 9650 4200
Connection ~ 9050 2650
Wire Wire Line
	9050 3800 9050 2650
Wire Wire Line
	10250 3550 10250 2650
Wire Wire Line
	10250 2650 2150 2650
Wire Wire Line
	2100 3850 2250 3850
Connection ~ 2250 4700
Wire Wire Line
	2250 3850 2250 4700
Connection ~ 4350 2650
Wire Wire Line
	4350 2350 4350 2650
Wire Wire Line
	4700 2100 5350 2100
Connection ~ 6700 4100
Wire Wire Line
	7250 4100 6700 4100
Wire Wire Line
	6700 3550 6350 3550
Connection ~ 7100 2650
Wire Wire Line
	7100 2650 7100 3250
Connection ~ 2600 2650
Wire Wire Line
	2600 3550 2600 2650
Wire Wire Line
	6300 4050 6350 4050
Wire Wire Line
	6350 4050 6350 3550
Wire Wire Line
	4800 3650 4500 3650
Connection ~ 5750 4700
Wire Wire Line
	5750 4700 5750 3950
Wire Wire Line
	3500 4150 3500 3750
Wire Wire Line
	3500 4150 4000 4150
Connection ~ 3900 4700
Connection ~ 3900 2650
Wire Wire Line
	3900 2650 3900 3250
Wire Wire Line
	3500 3750 3300 3750
Wire Wire Line
	3900 4050 3900 4850
Wire Wire Line
	3400 3550 3400 2100
Wire Wire Line
	3400 3550 3500 3550
Wire Wire Line
	4500 3650 4500 4150
Wire Wire Line
	4500 4150 4350 4150
Wire Wire Line
	5750 2650 5750 3150
Connection ~ 5750 2650
Wire Wire Line
	5150 3650 5350 3650
Wire Wire Line
	5950 4050 5350 4050
Wire Wire Line
	5350 4050 5350 3650
Wire Wire Line
	5350 2100 5350 3450
Wire Wire Line
	2800 3750 2950 3750
Wire Wire Line
	2600 4700 2600 3900
Connection ~ 2600 4700
Wire Wire Line
	7700 4100 7700 3650
Wire Wire Line
	7700 4100 7600 4100
Wire Wire Line
	6700 4700 6700 4600
Connection ~ 6700 4700
Wire Wire Line
	7100 4050 7100 4700
Connection ~ 7100 4700
Wire Wire Line
	6700 3750 6700 4250
Wire Wire Line
	3400 2100 4200 2100
Wire Wire Line
	4450 2350 4450 2450
Wire Wire Line
	2250 2650 2250 3700
Connection ~ 2250 2650
Wire Wire Line
	2250 3700 2100 3700
Wire Wire Line
	7700 3650 10250 3650
Wire Wire Line
	10250 3750 10250 4700
Wire Wire Line
	10250 4700 2000 4700
Wire Wire Line
	9050 4600 9050 4700
Connection ~ 9050 4700
Wire Wire Line
	8400 4050 8400 4150
Wire Wire Line
	8400 3700 8400 2650
Connection ~ 8400 2650
$Comp
L R-US R?
U 1 1 50A7B042
P 8300 4300
AR Path="/50A7B03C" Ref="R?"  Part="1" 
AR Path="/50A7B042" Ref="R8"  Part="1" 
F 0 "R8" H 8300 4300 60  0000 C CNN
F 1 "100K" H 8300 4100 60  0000 C CNN
	1    8300 4300
	0    -1   -1   0   
$EndComp
$Comp
L R-US R9
U 1 1 50A7B03C
P 8500 3900
F 0 "R9" H 8500 3900 60  0000 C CNN
F 1 "100K" H 8500 3700 60  0000 C CNN
	1    8500 3900
	0    1    1    0   
$EndComp
Text Notes 9300 4100 0    60   ~ 0
unused
$Comp
L LM324 U1
U 4 1 50A7AFB4
P 9150 4200
F 0 "U1" H 9200 4400 60  0000 C CNN
F 1 "LM324" H 9300 4000 50  0000 C CNN
	4    9150 4200
	1    0    0    -1  
$EndComp
$Comp
L CONN_3 K1
U 1 1 50A6A481
P 10600 3650
F 0 "K1" V 10550 3650 50  0000 C CNN
F 1 "CONN_3" V 10650 3650 40  0000 C CNN
	1    10600 3650
	1    0    0    -1  
$EndComp
$Comp
L CP C2
U 1 1 50A69E7B
P 2000 3800
F 0 "C2" H 2000 3800 60  0000 C CNN
F 1 "CP" H 2000 3800 60  0000 C CNN
	1    2000 3800
	0    -1   -1   0   
$EndComp
$Comp
L CP C1
U 1 1 50A69BA7
P 2150 3800
F 0 "C1" H 2150 3800 60  0000 C CNN
F 1 "CP" H 2150 3800 60  0000 C CNN
	1    2150 3800
	0    -1   -1   0   
$EndComp
$Comp
L GND #PWR3
U 1 1 50A5BE0E
P 4450 2450
F 0 "#PWR3" H 4450 2450 30  0001 C CNN
F 1 "GND" H 4450 2380 30  0001 C CNN
	1    4450 2450
	1    0    0    -1  
$EndComp
$Comp
L MPX2010 U2
U 1 1 50A5BDED
P 4450 1900
F 0 "U2" H 4450 1900 60  0000 C CNN
F 1 "MPX2010" H 4400 1650 60  0000 L CNN
	1    4450 1900
	1    0    0    -1  
$EndComp
$Comp
L LM324 U1
U 3 1 50A5BB70
P 7200 3650
F 0 "U1" H 7250 3850 60  0000 C CNN
F 1 "LM324" H 7350 3450 50  0000 C CNN
	3    7200 3650
	1    0    0    -1  
$EndComp
$Comp
L R-US R2
U 1 1 50A5BBE9
P 6600 4400
F 0 "R2" H 6600 4400 60  0000 C CNN
F 1 "47K" H 6600 4200 60  0000 C CNN
	1    6600 4400
	0    -1   -1   0   
$EndComp
$Comp
L R-US R7
U 1 1 50A5BBDC
P 7450 4000
F 0 "R7" H 7450 4000 60  0000 C CNN
F 1 "100K" H 7450 3800 60  0000 C CNN
	1    7450 4000
	1    0    0    -1  
$EndComp
$Comp
L R-ADJ-US-3PIN R1
U 1 1 50A5746E
P 2450 3700
F 0 "R1" H 2450 3650 60  0000 C CNN
F 1 "100K" H 2550 3400 60  0000 C CNN
	1    2450 3700
	0    -1   -1   0   
$EndComp
Text Notes 7800 3600 0    60   ~ 0
output
$Comp
L R-US R6
U 1 1 50A54811
P 6150 3950
F 0 "R6" H 6150 3950 60  0000 C CNN
F 1 "100K" H 6150 3750 60  0000 C CNN
	1    6150 3950
	1    0    0    -1  
$EndComp
$Comp
L R-US R5
U 1 1 50A54809
P 5000 3550
F 0 "R5" H 5000 3550 60  0000 C CNN
F 1 "1K" H 5000 3350 60  0000 C CNN
	1    5000 3550
	1    0    0    -1  
$EndComp
$Comp
L LM324 U1
U 1 1 50A547AA
P 5850 3550
F 0 "U1" H 5900 3750 60  0000 C CNN
F 1 "LM324" H 6000 3350 50  0000 C CNN
	1    5850 3550
	1    0    0    -1  
$EndComp
$Comp
L R-US R4
U 1 1 50A54751
P 4200 4050
F 0 "R4" H 4200 4050 60  0000 C CNN
F 1 "1K" H 4200 3850 60  0000 C CNN
	1    4200 4050
	1    0    0    -1  
$EndComp
$Comp
L LM324 U1
U 2 1 50A546E7
P 4000 3650
F 0 "U1" H 4050 3850 60  0000 C CNN
F 1 "LM324" H 4150 3450 50  0000 C CNN
	2    4000 3650
	1    0    0    -1  
$EndComp
$Comp
L R-US R3
U 1 1 50A54697
P 3150 3650
F 0 "R3" H 3150 3650 60  0000 C CNN
F 1 "100K" H 3150 3450 60  0000 C CNN
	1    3150 3650
	1    0    0    -1  
$EndComp
$Comp
L GND #PWR2
U 1 1 50A5468E
P 3900 4850
F 0 "#PWR2" H 3900 4850 30  0001 C CNN
F 1 "GND" H 3900 4780 30  0001 C CNN
	1    3900 4850
	1    0    0    -1  
$EndComp
$Comp
L +5V #PWR1
U 1 1 50A54679
P 2150 2650
F 0 "#PWR1" H 2150 2740 20  0001 C CNN
F 1 "+5V" H 2150 2740 30  0000 C CNN
	1    2150 2650
	0    -1   -1   0   
$EndComp
$EndSCHEMATC
