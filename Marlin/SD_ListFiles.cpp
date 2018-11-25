/*
- SD_ListFiles.cpp - A class that manages the parsing of time printing duration, filament consumption and if an gcode is a duplication print.
Last Update: 01/08/2018
Author: Alejandro Garcia (S3mt0x)
*/

#include "SD_ListFiles.h"

#include "genieArduino.h"
#include "Touch_Screen_Definitions.h"
#include "Marlin.h"
#include "Configuration.h"
#include "stepper.h"
#include "temperature.h"
#include "cardreader.h"
#include "BCN3D_customregisters.h"

Listfiles::Listfiles(){
	dias=-1, horas=-1, minutos=-1;
	segundos = -1;
	filmetros1 = 0 ,filmetros2=0;
	filgramos1 = 0 ,filgramos2=0;
	simplify3D = -1;
}


void Listfiles::get_lineduration(bool fromfilepoiter, char* name){
	if(!fromfilepoiter){
		card.openFile(name, true);
	}else{
		card.openFile(card.filename, true);
	}
	//card.openFile(card.filename, true);
	dias=-1, horas=-1, minutos=-1, segundos=-1;
	char serial_char='\0';
	int posi = 0;
	int linecomepoint=0;
	simplify3D=-1;
	while(linecomepoint < 5 && !card.isEndFile()){
		memset(commandline, '\0', sizeof(commandline) );
		if (search_line_data_commentary() == -1){
			linecomepoint = 5;
			dias=0;
			horas=0;
			minutos = 1;
			filgramos1 = 0;
			filgramos2 = 0;
			card.closefile();
			return;
		}
		if(linecomepoint == 0){
			if (extract_data_Simplify() !=-1 ){
				linecomepoint = 5;
				simplify3D = 1;
			}
			else{
				simplify3D = 0;
			}
		}
		else{
			extract_data();
			if (minutos !=-1 ){
				linecomepoint = 5;
			}else if(segundos !=-1){
				linecomepoint = 5;
			}
		}
		
		linecomepoint++;
	}
	//Serial.println(simplify3D);
	if(simplify3D == 0){
		
		memset(commandline, '\0', sizeof(commandline));
		posi = 0;
		serial_char='\0';
		
		while(serial_char != '\n' && posi < 49 && !card.isEndFile()){
			
			int16_t n=card.get();
			serial_char = (char)n;
			commandline[posi]=serial_char;
			posi++;
		}
		extract_data1();
		//card.closefile();
		if(minutos == -1 && segundos == -1){
			dias=0;
			horas=0;
			minutos = 1;
			filgramos1 = 0;
			filgramos2 = 0;
		}
		memset(commandline, '\0', sizeof(commandline) );
	}
	else if (simplify3D == 1){
		card.sdfinalline();
		memset(commandline, '\0', sizeof(commandline) );
		
		linecomepoint=0;
		while(linecomepoint < 5 && !card.isEndFile()){
			memset(commandline, '\0', sizeof(commandline) );
			if (search_line_data_commentary() == -1 ){
				linecomepoint = 5;
				dias=0;
				horas=0;
				minutos = 1;
				filgramos1 = 0;
				filgramos2 = 0;
				card.closefile();
				return;
			}
			extract_data();
			if(minutos !=-1)break;
			linecomepoint++;
		}
		if(minutos < 0){
			dias=0;
			horas=0;
			minutos = 1;
		}
		memset(commandline, '\0', sizeof(commandline) );
		posi = 0;
		serial_char='\0';
		linecomepoint = 0;
		while(linecomepoint < 5 && !card.isEndFile()){
			memset(commandline, '\0', sizeof(commandline) );
			if (search_line_data_commentary() == -1 ){
				linecomepoint = 5;
				dias=0;
				horas=0;
				minutos = 1;
				filgramos1 = 0;
				filgramos2 = 0;
				card.closefile();
				return;
			}
			extract_data1();
			if(filgramos1 !=-1)break;
			linecomepoint++;
		}
		if(filgramos1 < 0){
			filgramos1 = 0;
			filmetros2 = 0;
			filmetros1 = 0;
			filgramos2 = 0;
		}
		//card.closefile();
		memset(commandline, '\0', sizeof(commandline));
	}
	card.closefile();
	
	
	
}


int Listfiles::search_line_data_commentary(){
	char serial_char='\0';
	int posi = 0;
	int exit = 0;
	while(commandline[0]!=';' && !card.isEndFile()){
		serial_char='\0';
		posi = 0;
		while(serial_char != '\n' && posi < 49 && !card.isEndFile()){
			
			int16_t n=card.get();
			serial_char = (char)n;
			commandline[posi]=serial_char;
			
			posi++;
		}
		exit++;
		if(exit>20 || card.isEndFile()){
			return 1;
		}
	}
	return 0;
}
inline int Listfiles::search_line_data_code(char code){
	char serial_char='\0';
	int posi = 0;
	int exit = 0;
	while(commandline[0]!=code && !card.isEndFile()){
		serial_char='\0';
		posi = 0;
		while(serial_char != '\n' && posi < 49 && !card.isEndFile()){
			
			int16_t n=card.get();
			serial_char = (char)n;
			commandline[posi]=serial_char;
			
			posi++;
		}
		exit++;
		if(exit>300 || card.isEndFile()){
			return -1;
		}
	}
	return 0;
}
int Listfiles::extract_data_Simplify(void){
	int Simplify_ok =-1;
	sscanf_P( commandline, PSTR("; G-Code generated by Simplify3D(R) Version %d"), &Simplify_ok);
	return Simplify_ok;
}
inline void Listfiles::extract_data1(void){
	float calgramos = 0.0;
	char *strchr_pointer;
	filgramos1 = -1;
	filmetros2 = -1;
	filmetros1 = -1;
	filgramos2 = -1;
	int32_t metros = 0;
	int32_t metros2 = 0;
	if(simplify3D == 0){
		if(segundos !=-1){
			sscanf_P(commandline, PSTR(";Filament used: %ld.%ldm"), &metros, &metros2);
			if(metros<0 || metros2<0){metros=0;metros2=0;return;}
			strchr_pointer = strchr(commandline, ':');
			if(strchr_pointer != NULL){
			calgramos = strtod(&commandline[strchr_pointer - commandline + 1], NULL);
			//metros2 = get_firstdigit_from_integer(metros2);
			}else{
				calgramos = 0.0;
			}
			calgramos = (float) 7.974264375 * calgramos;//filamentDensity = 1.25; distanceMultiplier = pi * (2.85/2)^2 * filamentDensity; grams = distanceFromGcodeInMeters * distanceMultiplier
			filgramos1 = (int) round(calgramos);
			
			}else{
			sscanf_P(commandline, PSTR(";Filament used: %d.%dm %d.%dg"), &filmetros1, &filmetros2, &filgramos1, &filgramos2);
			if(filgramos1 < 0){
				filgramos1 = 0;
				filmetros2 = 0;
				filmetros1 = 0;
				filgramos2 = 0;
			}
		}
		
	}
	else if ( simplify3D == 1){
		sscanf_P(commandline, PSTR(";   Plastic weight: %d.%dg"), &filgramos1, &filgramos2);
		
	}

}
int Listfiles::check_extract_match_hotendsize_print(void){
	
	float hotend0 = 0.0;
	float hotend1 = 0.0;
	card.openFile(card.filename, true);

	int linecomepoint=0;
	simplify3D=-1;
	while(linecomepoint < 30 && !card.isEndFile()){
		memset(commandline, '\0', sizeof(commandline) );
		if (search_line_data_commentary() == -1){
			dias=0;
			horas=0;
			minutos = 1;
			filgramos1 = 0;
			filgramos2 = 0;
			card.closefile();
			return 1;
		}
		if(linecomepoint == 0){
			if (extract_data_Simplify() !=-1 ){
				linecomepoint = 7;
				simplify3D = 1;
			}
			else{
				simplify3D = 0;
			}
		}
		else{
			hotend0 = extract_ensure_hotend_gcode_setup_t(LEFT_EXTRUDER);
			hotend1 = extract_ensure_hotend_gcode_setup_t(RIGHT_EXTRUDER);
			if (hotend0 > 0 || hotend1 > 0){
				break;
			}
		}
		
		linecomepoint++;
	}
	card.closefile();
	if (hotend0 <= 0 && hotend1 <= 0){
		return 1;
	}else if(hotend0 == hotend_size_setup[0] && hotend0 > 0 && hotend1 <= 0){
		return 2;
	}else if(hotend1 == hotend_size_setup[1] && hotend1 > 0 && hotend0 <= 0){
		return 3;
	}else if(hotend0 == hotend_size_setup[0] && hotend1 == hotend_size_setup[1]){
		return 4;
	}else{
		which_hotend_setup[LEFT_EXTRUDER] = (int) ((hotend0 <= 0 ? hotend_size_setup[0]:hotend0) * 10);
		which_hotend_setup[RIGHT_EXTRUDER] = (int) ((hotend1 <= 0 ? hotend_size_setup[1]:hotend1) * 10);
		return 0;
	}
	
}
int Listfiles::check_extract_ensure_duplication_print(void){
	
	
	card.openFile(card.filename, true);

	int linecomepoint=0;
	simplify3D=-1;
	while(linecomepoint < 15 && !card.isEndFile()){
		memset(commandline, '\0', sizeof(commandline) );
		if (search_line_data_code('M') == -1){
			card.closefile();
			return 0;
		}
		if(linecomepoint == 0){
			if (extract_data_Simplify() !=-1 ){
				simplify3D = 1;
			}
			else{
				simplify3D = 0;
			}
		}
		if ((extract_ensure_duplication_print() == 5 || extract_ensure_duplication_print() == 6)){
			linecomepoint = 15;
		}
		if (linecomepoint == 14){
			return 0;
		}
		
		linecomepoint++;

	}
	card.setIndex(0);// go to init
	linecomepoint = 0;
	if(simplify3D){
		while(linecomepoint < 250 && !card.isEndFile()){
			memset(commandline, '\0', sizeof(commandline) );
			if (search_line_data_commentary() == -1){
				card.closefile();
				return 0;
			}
			if(extract_ensure_duplication_print_with_raft_simplify() != -1){
				if(extract_ensure_duplication_print_with_raft_simplify() == 0){
					card.closefile();
					return 1;
					}else{
					card.closefile();
					return 0;
				}
			}
			linecomepoint++;

		}
		
		}else{
		while(linecomepoint < 60 && !card.isEndFile()){
			memset(commandline, '\0', sizeof(commandline) );
			if (search_line_data_commentary() == -1){
				card.closefile();
				return 0;
			}
			if(extract_ensure_duplication_print_with_raft_cura() < 0){
				card.closefile();
				return 0;
			}
			linecomepoint++;

		}
		
	}
	card.closefile();
	return 1;
}
float Listfiles::extract_ensure_hotend_gcode_setup_t(uint8_t tool){
	int nozzle_0 =-1;
	int nozzle_1 =-1;
	int nozzle_digit1 =-1;
	int nozzle_digit2 =-1;
	int nozzle_digit3 =-1;
	int nozzle_digit4 =-1;
	float res=0.0;
	if(simplify3D) sscanf_P( commandline, PSTR(";   extruderDiameter,%d.%d,%d.%d"), &nozzle_digit1, &nozzle_digit2, &nozzle_digit3, &nozzle_digit4);
	else sscanf_P( commandline, PSTR(";Extruders used: T%d %d.%d T%d %d.%d"), &nozzle_0, &nozzle_digit1, &nozzle_digit2, &nozzle_1, &nozzle_digit3, &nozzle_digit4);
	if(nozzle_0 == 1){
		nozzle_digit1 = -1;
		nozzle_digit2 = -1;
	}
	if(nozzle_1 == 0){
		nozzle_digit1 = nozzle_digit3;
		nozzle_digit2 = nozzle_digit4;
	}
	if(tool == LEFT_EXTRUDER){
		if(nozzle_digit1==1 && nozzle_digit2 == -1)nozzle_digit2=0;
		res = (float) (nozzle_digit1 + nozzle_digit2/10.0);
	}else{
		if(nozzle_digit3==1 && nozzle_digit4 == -1)nozzle_digit4=0;
		res = (float) (nozzle_digit3 + nozzle_digit4/10.0);
	}
	
	return res;
}
int Listfiles::extract_ensure_duplication_print(void){
	int dupli_flag_print =-1;
	sscanf_P( commandline, PSTR("M605 S%d"), &dupli_flag_print);
	return dupli_flag_print;
}
int Listfiles::extract_ensure_duplication_print_with_raft_simplify(void){
	int dupli_flag_print_raft =-1;
	sscanf_P( commandline, PSTR(";   useRaft,%d"), &dupli_flag_print_raft);
	return dupli_flag_print_raft;
}
int Listfiles::extract_ensure_duplication_print_with_raft_cura(void){
	int dupli_flag_print_raft = 0;
	sscanf_P( commandline, PSTR(";LAYER:%d"), &dupli_flag_print_raft);
	return dupli_flag_print_raft;
}
uint32_t Listfiles::get_firstdigit_from_integer(uint32_t num_input){
uint32_t num = num_input;
uint32_t digit = 0;
	while(num != 0)
	{
		digit = num % 10;
		num = num / 10;
	}
	return digit;
}
inline void Listfiles::extract_data(void){
	dias=-1, horas=-1, minutos= -1, segundos= -1;
	char a[10],b[10],c[10];
	if (simplify3D == 0){
		sscanf_P(commandline, PSTR(";TIME:%ld"), &segundos);
		if(segundos < 0){
			segundos = -1;
			sscanf_P(commandline, PSTR(";Print time: %d %s %d %s %d %s"), &dias, a, &horas, b, &minutos, c);
			if(dias !=-1){
				//if(minutos !=-1)return;
				if(minutos !=-1);
				else{
					if(horas !=-1){
						minutos = horas;
						horas = dias;
						dias = 0;
						//return;
						}else{
						minutos = dias;
						horas = 0;
						dias = 0;
					}
				}
			}
			}else{
			horas = 0;
			minutos = 0;
			dias = 0;
		}
	}
	else if (simplify3D == 1){
		dias = 0;
		sscanf_P( commandline, PSTR(";   Build time: %d %s %d %s\n"), &horas, a, &minutos, b);
				
	}
	
}

int Listfiles::get_hours(){
	long hours = 0;
	if(dias>0){
		hours = 24*dias + horas;
	}
	else{
		hours = horas + segundos/3600;
	}
	return (int)hours;
}
int Listfiles::get_minutes(){
	long mins = 0;
	mins = minutos + (segundos/60)%60;
	return (int)mins;
}
inline int Listfiles::get_filmetros1(){
	int metr = 0;
	metr = filmetros1;
	return metr;
}
int Listfiles::get_filmetros2(){
	int metr = 0;
	metr = filmetros2;
	return metr;
}
int Listfiles::get_filgramos1(){
	int gram = 0;
	gram = filgramos1;
	return gram;
}
int Listfiles::get_filgramos2(){
	int gram = 0;
	gram = filgramos2;
	return gram;
}
int Listfiles::get_hoursremaining(){
	unsigned long long hours = 0;
	#ifdef ENABLE_CURA_COUNTDOWN_TIMER
	if(!flag_is_cura_file){
		if (get_hours()==0)return 0;
		hours =(unsigned long long)get_hours()*60+(unsigned long long)get_minutes();
		hours = hours-hours*card.getSdPosition()/card.getFileSize();
		hours = hours/60;
	}else{
		hours = (((unsigned long long)(is_cura_file_total_time - is_cura_file_total_timeelapsed))/3600);
	}
	#else
	if (get_hours()==0)return 0;
	hours =(unsigned long long)get_hours()*60+(unsigned long long)get_minutes();
	hours = hours-hours*card.getSdPosition()/card.getFileSize();
	hours = hours/60;
	#endif
	return (int) hours;
}
int Listfiles::get_minutesremaining(){
	unsigned long long minu = 0;
	#ifdef ENABLE_CURA_COUNTDOWN_TIMER
	if(!flag_is_cura_file){
	
	if (get_minutes()==-1)return 0;
		
	}else{
		minu = (((unsigned long long)(is_cura_file_total_time - is_cura_file_total_timeelapsed))/60)%60;
		
	}
	#else
	minu = (unsigned long long)get_hours()*60+(unsigned long long)get_minutes();
	minu = minu-minu*card.getSdPosition()/card.getFileSize();
	minu = minu%60;
	#endif
	return (int) minu;
}
int Listfiles::get_hoursremaining_save(long position){
	unsigned long long hours = 0;
	
	if (get_hours()==0)return 0;
	hours =(unsigned long long)get_hours()*60+(unsigned long long)get_minutes();
	Serial.println((long)hours);
	hours = hours-hours*position/card.getFileSize();
	Serial.println((long)hours);
	hours = hours/60;
	Serial.println((long)hours);
	
	return (int) hours;
}
int Listfiles::get_minutesremaining_save(long position){
	unsigned long long minu = 0;
	if (get_minutes()==-1)return 0;
	minu = (unsigned long long)get_hours()*60+(unsigned long long)get_minutes();
	minu = minu-minu*position/card.getFileSize();
	minu = minu%60;
	return (int) minu;
}
int Listfiles::get_percentage_save(long position){
	unsigned long long minu = 0;
	minu = position*100/card.getFileSize();
	return (int) minu;
}