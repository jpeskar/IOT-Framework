#include <Battery.h>
#include <Arduino.h>
#include <stdint.h>

/* **************************************************
   Name:        BatteryMeasurement

   Returns:     floating point voltage as mV

   Parameters:   None

   Description: This function call the function to determine the actual voltage.
          An ADC is run on a voltage divider and is used with the actual
          chip voltage to conversion is used calculate the voltage of the battery.
 ************************************************** */
float Battery::BatteryMeasurement(int pin){
  float    voltage;
  uint16_t ADC;
  uint16_t realVcc;
  realVcc = ReadVcc();

  for (int i =1 ; i < 6; i++){ //read 6 ADC values toss the first
     ADC += analogRead(pin);
  }
  ADC /= 5;
  voltage = ((float)ADC * realVcc * 3.128) / 1023.0;
return voltage;
}

/* **************************************************
   Name:        ReadVcc

   Accepts:		Nothing

   Returns:     returns an unsigned integer of millivolts

   Parameters:

   Description: This function uses the internal 1.1 voltage reference and compares it to
   		the Vcc of the chip. It uses the difference to calculate the real Vcc. Very
   		useful in determining accurate ADC's. Not my function... found it on the web.
************************************************** */
int Battery::ReadVcc() {
	uint16_t result;
	uint8_t  saveADMUX;

	saveADMUX = ADMUX;

	// Read 1.1V reference against AVcc
	ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
	delay(2); // Wait for Vref to settle
	ADCSRA |= _BV(ADSC); // Convert
	while (bit_is_set(ADCSRA, ADSC));
	result = ADCL;
	result |= ADCH << 8;
	result = 1126400L / result; // Back-calculate AVcc in mV

	ADMUX = saveADMUX;  // restore it on exit...
	return result;
}

Battery battery;