/**********************************************************************
 BrewManiac 
 created by Vito Tai
 Copyright (C) 2015 Vito Tai
 
 This soft ware is provided as-is. Use at your own risks.
 You are free to modify and distribute this software without removing 
 this statement.
 BrewManiac by Vito Tai is licensed under a Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License.
***********************************************************************/

#ifndef MY_STRING_LIB
#define MY_STRING_LIB

byte sprintIntDigit(char *buff, int number,int base)
{
    byte indx=0;
    while (base > 1)
    {
       	int digit= number / base;
       	buff[indx++]=('0' + digit);
       	number = number - digit  * base;
    	base /= 10;

    }
	buff[indx++]=('0' + number);
   	return indx;
}

byte sprintInt(char *buff,int number)
{
	byte sign=0;
	if(number <0)
	{
		*(buff++)='-';
		//buff = buff +1;
		number = -number;
		sign = 1;
	}
	int base=1;
	
  	if(number >= 10)
  	{
    	while( number >= base)
    	{
     		base *= 10;
    	}
    	base /= 10;
 	}
 	return sign+sprintIntDigit(buff,number,base);
}

byte sprintFloat(char *buff,float value,byte precision)
{
    byte len=0;

	if(value <0)
	{
		*(buff++)='-';
		//buff = buff +1;
		value = -value;
		len=1;
	}

   	int base=1;
    float r=0.5;
   	for(byte p=0; p < precision; p++)
   	{
      	base *= 10;
        r *= 0.1;
   	}

 	float number = value+ r;   
   	if (number >= 1.0) 
   	{
     	int integer=(int)number;
     	len +=sprintInt(buff,integer);
     	number -= (float)integer;
   	}
   	else
   	{
       	buff[0]='0';
       	len += 1;
   	}
  
   	if(precision == 0) return len;
   	buff[len++]='.';
   
   	number = number * base;
   	len+=sprintIntDigit(buff+len,(int)number,base/10);
   	return len;
}

#endif
