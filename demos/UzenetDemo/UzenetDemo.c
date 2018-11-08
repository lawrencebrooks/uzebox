/*
 *  Uzebox quick and dirty tutorial
 *  Copyright (C) 2008  Alec Bourque
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdbool.h>
#include <avr/io.h>
#include <util/atomic.h> 
#include <stdlib.h>
#include <stdio.h>
#include <avr/pgmspace.h>
#include <time.h>
#include <uzebox.h>
#include "keyboard.h"
#include "console.h"
#include "uzenet.h"

#include "data/font6x8-full-mode9.inc"
#include "data/power-mode9.inc"
#include "data/patches.inc"
#include "data/stage4song.inc"


HttpResponse resp;


void timeout(){
	printf_P(PSTR("Timeout!\r\n"));

	u16 joy;
	while(true){

		while(UartUnreadCount()>0){
			putchar(UartReadChar());
		}

		joy=ReadJoypad(0);
		if(joy!=0){
			while(ReadJoypad(0)!=0);
			switch(joy){
				case BTN_Y:
				putchar('*');
				//debug_hex(uart_tx_tail);
				//debug_hex(uart_tx_head);
				putchar(0xa);
				break;

			}
		}
	}
}

void wifiHWReset(){
    //reset module
    DDRD|=(1<<PD3);
    PORTD&=~(1<<PD3);
    WaitVsync(1);
    PORTD|=(1<<PD3);
}

int main(){
	u8 c=0;
	char rxbuf[256];
    char mac[9];
	resp.content=rxbuf;


	SetCursorParams('z'-32+2,30);
	MoveCursor(0,0);
	SetCursorVisible(true);

	SetUserPreVsyncCallback(&ConsoleHandler);

	SetTileTable(font6x8);
	console_init(CONS_OUTPUT_NON_ALPHA);

	wifi_Echo(true);

	printf_P(PSTR("             *** Uzenet Console 0.1 ***\n\n"));
	printf_P(PSTR("For console commands type: help\n\n"));


    //printf_P(PSTR("Initializing UART...\r\n"));

	//initialize UART0 
	// UBRR0H=0;	

	//http://wormfood.net/avrbaudcalc.php
	//This is for single speed mode. Double the
	//values for UART double speed mode.
	//Baud  UBRR0L	Error%
	//9600	185		0.2
	//14400	123		0.2
	//19200	92		0.2
	//28800	61		0.2
	//38400	46		0.8
	//57600	30		0.2
	//76800	22		1.3
	//115200	15		3.0


	//UBRR0L=185;	//9600 bauds	960 bytes/s		16 bytes/field
	//UBRR0L=92;	//19200 bauds	1920 bytes/s	32 bytes/field
	//UBRR0L=46;	//38400 bauds	3840 bytes/s	64 bytes/field
	//UBRR0L=60;		//57600 bauds	5760 bytes/s	96 bytes/field (Double Speed)
	//UBRR0L=30;		//115200 bauds (Double Speed)
    //UBRR0L=60;

    printf_P(PSTR("Resetting module...\r\n"));
    wifiHWReset();

    u8 counter = 0;
    while(counter++ <= 61) WaitVsync(1);

	printf_P(PSTR("Initialize WIFI module...\r\n"));
	
   if(initWifi()!=WIFI_OK)timeout();

	//SendCommandAndWait(PSTR("ATE0\r\n"),PSTR("OK\r\n"));

    printf_P(PSTR("Firmware Version....\n"));
    SendCommandAndWait(PSTR("AT+GMR\r\n"),PSTR("OK\r\n"));

	u16 joy;
	while(true){

		while(UartUnreadCount()>0){
			c=UartReadChar();
			putchar(c);
		}
		
		joy=ReadJoypad(0);
		if(joy!=0){
			while(ReadJoypad(0)!=0);
			switch(joy){
				case BTN_A:
					//SendDataAndWait(PSTR("AT+CIPSEND=0,16\r\nOk, no problems!"),PSTR("OK\r\n"));
                    printf_P(PSTR("Set SoftAP mode...\r\n"));
                    SendCommandAndWait(PSTR("AT+CWMODE_CUR=2\r\n"),PSTR("OK\r\n"));

                    //printf_P(PSTR("Disable DHCP...\r\n"));
                    //SendCommandAndWait(PSTR("AT+CWDHCP_CUR=0,0\r\n"),PSTR("OK\r\n"));

                    printf_P(PSTR("Set SopftAP IP address\r\n"));
                    SendCommandAndWait(PSTR("AT+CIPAP_CUR=\"192.168.4.1\"\r\n"), PSTR("OK\r\n"));

                    printf_P(PSTR("Get SoftAP MAC address...\r\n"));
                    wifi_SendString_P(PSTR("AT+CIPAPMAC_CUR?\r\n"));
                    if (wifi_WaitForStringP(PSTR("OK\r\n"), rxbuf) != WIFI_TIMEOUT) {
                        mac[0] = 'T';
                        mac[1] = 'F';
                        u8 idx = 2;
                        for (u8 i = 43; i <= 49; i += 3) {
                           mac[idx++] = rxbuf[i]; 
                           mac[idx++] = rxbuf[i+1];
                        }
                        mac[8] = 0;
                        printf("%s\r\n", mac);
                    }

                    printf_P(PSTR("Setup local access point...\r\n"));
                    SendCommandAndWait(PSTR("AT+CWSAP_CUR=\"TFB9EEC2\",\"test12345\",5,3,1\r\n"),PSTR("OK\r\n"));

                    printf_P(PSTR("Activate UDP Passthrough mode\r\n"));
                    SendCommandAndWait(PSTR("AT+CIPSTART=\"UDP\",\"192.168.4.2\",1001,2233,0\r\n"), PSTR("OK\r\n"));

                    SendCommandAndWait(PSTR("AT+CIPMODE=1\r\n"),PSTR("OK\r\n"));
                    wifi_SendString_P(PSTR("AT+CIPSEND\r\n"));
				break;
				
				case BTN_B:
                    wifi_SendString_P(PSTR("abcdefghijklmnopqrstuvwxyzzz"));
					//SendDataAndWait(PSTR("AT+CIPSEND=0,13\r\nAnybody here?"),PSTR("OK\r\n"));
				break;
				
				case BTN_X:
                    printf_P(PSTR("Set Station mode...\r\n"));
                    SendCommandAndWait(PSTR("AT+CWMODE_CUR=1\r\n"),PSTR("OK\r\n"));
                    
                    //printf_P(PSTR("Disable DHCP...\r\n"));
                    //SendCommandAndWait(PSTR("AT+CWDHCP_CUR=1,0\r\n"),PSTR("OK\r\n"));
                    
                    printf_P(PSTR("Connect to access point...\r\n"));
                    SendCommandAndWait(PSTR("AT+CWJAP_CUR=\"TFB9EEC2\",\"test12345\"\r\n"),PSTR("OK\r\n"));
                    
                    printf_P(PSTR("Set station IP address"));
                    SendCommandAndWait(PSTR("AT+CIPSTA_CUR=\"192.168.4.2\"\r\n"), PSTR("OK\r\n"));
                    
                    printf_P(PSTR("Activate UDP Passthrough mode\r\n"));
                    SendCommandAndWait(PSTR("AT+CIPSTART=\"UDP\",\"192.168.4.1\",2233,1001\r\n"),PSTR("OK\r\n"));
                    SendCommandAndWait(PSTR("AT+CIPMODE=1\r\n"),PSTR("OK\r\n"));
                    wifi_SendString_P(PSTR("AT+CIPSEND\r\n"));
				break;
				
				case BTN_Y:
					//SendDataAndWait(PSTR("AT+CIPSEND=0,3\r\nYo!"),PSTR("OK\r\n"));
                    wifi_SendString_P(PSTR("+++"));
                    u8 counter = 0;
                    while(counter++ < 5) WaitUs(65535);
                    counter = 0;
                    SendCommandAndWait(PSTR("AT+CIPMODE=0\r\n"),PSTR("OK\r\n"));
                    SendCommandAndWait(PSTR("AT+CIPCLOSE\r\n"),PSTR("OK\r\n"));
				break;
				
			}
		}
	
		
	}



} 
