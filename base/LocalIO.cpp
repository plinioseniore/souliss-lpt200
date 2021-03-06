/**************************************************************************
	Souliss
    Copyright (C) 2011  Veseo

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
	
	Originally developed by Dario Di Maio
	
***************************************************************************/
/*!
    \file 
    \ingroup
*/

#include "Souliss.h"
#include "Typicals.h"
#include <Arduino.h>

#include "src/types.h"
#include "GetConfig.h"			// need : ethUsrCfg.h, vNetCfg.h, SoulissCfg.h, MaCacoCfg.h
#include "Souliss.h"

#define	PINRESET	0x0
#define	PINSET		0x1
#define	PINACTIVE	0x2
#define	PINRELEASED	0x3

U8 InPin[MAXINPIN];
static unsigned long time;

/**************************************************************************
/*!
	Reset output in memory map, this shall be used in case of LinkIO method
*/	
/**************************************************************************/
void Souliss_ResetOutput(U8 *memory_map, U8 slot)
{
	memory_map[MaCaco_OUT_s + slot] = 0x00;
}

/**************************************************************************
/*!
	Reset input in memory map, this shall be used in case of RemoteInput 
	method
*/	
/**************************************************************************/
void Souliss_ResetInput(U8 *memory_map, U8 slot)
{
	memory_map[MaCaco_IN_s + slot] = 0x00;
}

/**************************************************************************
/*!
	Create a link between input and output on the same memory map
	
	This method is useful if data shall be transferred from a device to
	another, in that case is not allowed for a receiver node to subscribe
	the inputs of another node.
	A receiver node can subscribe instead the outputs of another node, so
	linking input and output allow a transfer of data from inputs to another
	device via subscription mode.
	
	This method require also Souliss_ResetOutput.
*/	
/**************************************************************************/
void Souliss_LinkIO(U8 *memory_map, U8 input_slot, U8 output_slot, U8 *trigger)
{
	if(memory_map[MaCaco_IN_s + input_slot] != 0x00)
	{	
		memory_map[MaCaco_OUT_s + output_slot] = memory_map[MaCaco_IN_s + input_slot];	// Link IO
		memory_map[MaCaco_IN_s + input_slot] = Souliss_RstCmd;							// Reset
		*trigger = Souliss_TRIGGED;														// Trig change
	}						
}

/**************************************************************************
/*!
	Create a link between output and input on the same memory map
*/	
/**************************************************************************/
void Souliss_LinkOI(U8 *memory_map, U8 input_slot, U8 output_slot)
{
	if(memory_map[MaCaco_OUT_s + output_slot] != 0x00)
		memory_map[MaCaco_IN_s + input_slot] = memory_map[MaCaco_OUT_s + output_slot];	// Link IO
}

/**************************************************************************
/*!
	Link an hardware pin to the shared memory map, active on rising edge
	
	It write directly into the inputs map of the node, these data shall be
	used for logic applications.
*/	
/**************************************************************************/
U8 Souliss_DigIn(U8 pin, U8 value, U8 *memory_map, U8 slot, bool filteractive=false)
{
	// If pin is ON, set the flag. If at next cycle the pin will still
	// be ON the requested action will be performed
	if(digitalRead(pin) && (InPin[pin]==PINRESET))
	{
		InPin[pin] = PINSET;

		// Copy the value in the memory map
		if(!filteractive && memory_map)
		{
			memory_map[MaCaco_IN_s + slot] = value;	
			return value;		
		}
		
	}	
	else if(filteractive && digitalRead(pin) && InPin[pin]==PINSET)
	{
		// Flag that action is executed
		InPin[pin] = PINACTIVE;

		// Copy the value in the memory map
		if(memory_map)	
		{
			memory_map[MaCaco_IN_s + slot] = value;	
			return value;
		}
	}
	else if(filteractive && !digitalRead(pin) && InPin[pin]==PINACTIVE)
		InPin[pin] = PINRELEASED;
	else if(filteractive && !digitalRead(pin) && InPin[pin]==PINRELEASED)
		InPin[pin] = PINRESET;
	else if(!filteractive && !digitalRead(pin))
		InPin[pin] = PINRESET;
	
	return MaCaco_NODATACHANGED;
}

/**************************************************************************
/*!
	Link an hardware pin to the shared memory map, active on falling edge
	
	It write directly into the inputs map of the node, these data shall be
	used for logic applications.
*/	
/**************************************************************************/
U8 Souliss_LowDigIn(U8 pin, U8 value, U8 *memory_map, U8 slot, bool filteractive=false)
{
	// If pin is ON, set the flag. If at next cycle the pin will still
	// be ON the requested action will be performed
	if(!digitalRead(pin) && (InPin[pin]==PINRESET))
	{
		InPin[pin] = PINSET;

		// Copy the value in the memory map
		if(!filteractive && memory_map)
		{
			memory_map[MaCaco_IN_s + slot] = value;	
			return value;		
		}		
	}	
	else if(filteractive && !digitalRead(pin) && InPin[pin]==PINSET)
	{
		// Flag that action is executed
		InPin[pin] = PINACTIVE;

		// Copy the value in the memory map
		if(memory_map)	
		{
			memory_map[MaCaco_IN_s + slot] = value;	
			return value;
		}
	}
	else if(filteractive && digitalRead(pin) && InPin[pin]==PINACTIVE)
		InPin[pin] = PINRELEASED;
	else if(filteractive && digitalRead(pin) && InPin[pin]==PINRELEASED)
		InPin[pin] = PINRESET;
	else if(!filteractive && digitalRead(pin))
		InPin[pin] = PINRESET;
	
	return MaCaco_NODATACHANGED;
}

/**************************************************************************
/*!
	Link an hardware pin to the shared memory map, use with latched two state
	pushbutton.
*/	
/**************************************************************************/
U8 Souliss_DigIn2State(U8 pin, U8 value_state_on, U8 value_state_off, U8 *memory_map, U8 slot)
{
	// If pin is on, set the "value"
	if(digitalRead(pin) && !InPin[pin])
	{	
		if(memory_map)	memory_map[MaCaco_IN_s + slot] = value_state_on;
		
		InPin[pin] = PINSET;
		return value_state_on;
	}
	else if(!digitalRead(pin) && InPin[pin])
	{
		if(memory_map)	memory_map[MaCaco_IN_s + slot] = value_state_off;
		
		InPin[pin] = PINRESET;
		return value_state_off;
	}
	
	return MaCaco_NODATACHANGED;
}

/**************************************************************************
/*!
	Use a single analog input connected to two different pushbuttons, use 
	different pull-up resistors to define different voltage drops for the
	two pushbuttons.
	
	If the analog value goes over the top limit or below the bottom one,
	the pushbuttons are pressed, if the analog value stay in the middle 
	no action is taken.
*/	
/**************************************************************************/
U8 Souliss_AnalogIn2Buttons(U8 pin, U8 value_button1, U8 value_button2, U8 *memory_map, U8 slot)
{
	uint16_t iPinValue = 0;  
	bool bState=false;
	bool bMiddle=false;

	iPinValue = analogRead(pin);    

	if (iPinValue >= AIN2S_TOP)
	{
	  bState=true;
	  bMiddle=false;
	}
	else if (iPinValue <= AIN2S_BOTTOM)
	{
	  bState=false;
	  bMiddle=false;
	}
	else 
		bMiddle=true;


	// If pin is on, set the "value"
    if(bState && !InPin[pin] && !bMiddle)
    {    
        if(memory_map)	memory_map[MaCaco_IN_s + slot] = value_button1;
        
		InPin[pin] = PINSET;
        return value_button1;
    }
    else if(!bState && !InPin[pin] && !bMiddle)
    {
        if(memory_map)	memory_map[MaCaco_IN_s + slot] = value_button2;
        
		InPin[pin] = PINSET;
        return value_button2;
    }
	else if(bMiddle) 
		InPin[pin] = PINRESET;

	return MaCaco_NODATACHANGED;
}

/**************************************************************************
/*!
	Link an hardware pin to the shared memory map, use with latched two state
	pushbutton, active on falling edge
*/	
/**************************************************************************/
U8 Souliss_LowDigIn2State(U8 pin, U8 value_state_on, U8 value_state_off, U8 *memory_map, U8 slot)
{
	// If pin is off, set the "value"
	if(digitalRead(pin)==0 && !InPin[pin])
	{
		if(memory_map)	memory_map[MaCaco_IN_s + slot] = value_state_on;
	 
		InPin[pin] = PINSET;
		return value_state_on;
	}
	else if(digitalRead(pin) && InPin[pin])
	{
		if(memory_map)	memory_map[MaCaco_IN_s + slot] = value_state_off;
	 
		InPin[pin] = PINRESET;
		return value_state_off;
	}
	 
	return MaCaco_NODATACHANGED;
}

/**************************************************************************
/*!
	Link an hardware pin to the shared memory map, active on rising edge
	Identify two states, press and hold.
*/	
/**************************************************************************/
U8 Souliss_DigInHold(U8 pin, U8 value, U8 value_hold, U8 *memory_map, U8 slot, U16 holdtime=1500)
{
	// If pin is on, set the "value"
	if(digitalRead(pin) && (InPin[pin]==PINRESET))
	{
		time = millis();								// Record time
		InPin[pin] = PINSET;
		
		return MaCaco_NODATACHANGED;
	}
	else if(digitalRead(pin) && (abs(millis()-time) > holdtime) && (InPin[pin]==PINSET))
	{
		InPin[pin] = PINACTIVE;								// Stay there till pushbutton is released
		
		// Write timer value in memory map
		if(memory_map)	memory_map[MaCaco_IN_s + slot] = value_hold;

		return value_hold;
	}
	else if(!digitalRead(pin) && (InPin[pin]==PINSET))
	{
		// Write input value in memory map
		if(memory_map)	memory_map[MaCaco_IN_s + slot] = value;
	
		InPin[pin] = PINRESET;
		return value;
	}
	else if(!digitalRead(pin) && (InPin[pin]==PINACTIVE))
		InPin[pin] = PINRESET;		
	
	return MaCaco_NODATACHANGED;
}

/**************************************************************************
/*!
	Link an hardware pin to the shared memory map, active on falling edge
	Identify two states, press and hold.
*/	
/**************************************************************************/
U8 Souliss_LowDigInHold(U8 pin, U8 value, U8 value_hold, U8 *memory_map, U8 slot, U16 holdtime=1500)
{
	// If pin is on, set the "value"
	if(!digitalRead(pin) && !InPin[pin])
	{
		time = millis();								// Record time
		
		InPin[pin] = PINSET;
		return MaCaco_NODATACHANGED;
	}
	else if(!digitalRead(pin) && (abs(millis()-time) > holdtime) && (InPin[pin]==PINSET))
	{
		InPin[pin] = PINRESET;								// Stay there till pushbutton is released
		
		// Write timer value in memory map
		if(memory_map)	memory_map[MaCaco_IN_s + slot] = value_hold;
		
		return value_hold;
	}
	else if(digitalRead(pin) && (InPin[pin]==PINSET))
	{
		// Write input value in memory map
		if(memory_map)	memory_map[MaCaco_IN_s + slot] = value;
	
		InPin[pin] = PINRESET;
		return value;
	}
	
	return MaCaco_NODATACHANGED;
}

/**************************************************************************
/*!
	Read a single precision floating point and store it into the memory_map 
	as half-precision floating point
*/	
/**************************************************************************/
void Souliss_ImportAnalog(U8* memory_map, U8 slot, float* analogvalue)
{
	float16((U16*)(memory_map + MaCaco_IN_s + slot), analogvalue);
}

/**************************************************************************
/*!
	Read an analog input and store it into the memory_map as half-precision
	floating point
*/	
/**************************************************************************/
void Souliss_AnalogIn(U8 pin, U8 *memory_map, U8 slot, float scaling, float bias)
{
	float inval = analogRead(pin);
	
	// Scale and add bias
	inval = bias + scaling * inval;
	
	// Convert from single-precision to half-precision
	float16((U16*)(memory_map + MaCaco_IN_s + slot), &inval);
	
}

/**************************************************************************
/*!
	Link the shared memory map to an hardware pin
	
	It write a digital output pin based on the value of the output into
	memory_map, let a logic act on external devices.
*/	
/**************************************************************************/
void Souliss_DigOut(U8 pin, U8 value, U8 *memory_map, U8 slot)
{
	// If output is active switch on the pin, else off
	if(memory_map[MaCaco_OUT_s + slot] == value)
		digitalWrite(pin, HIGH);
	else
		digitalWrite(pin, LOW);
}

/**************************************************************************
/*!
	Link the shared memory map to an hardware pin
	
	It write a digital output pin based on the value of the output into
	memory_map, let a logic act on external devices.
*/	
/**************************************************************************/
void Souliss_LowDigOut(U8 pin, U8 value, U8 *memory_map, U8 slot)
{
	// If output is active switch on the pin, else off
	if(memory_map[MaCaco_OUT_s + slot] == value)
		digitalWrite(pin, LOW);
	else
		digitalWrite(pin, HIGH);
}

/**************************************************************************
/*!
	Link the shared memory map to an hardware pin
	
	It write a digital output pin based on the value of the output into
	memory_map, let a logic act on external devices. Match criteria is based 
	on bit-wise AND operation.
*/	
/**************************************************************************/
void Souliss_nDigOut(U8 pin, U8 value, U8 *memory_map, U8 slot)
{
	// If output is active switch on the pin, else off
	if(memory_map[MaCaco_OUT_s + slot] & value)
		digitalWrite(pin, HIGH);
	else
		digitalWrite(pin, LOW);
}

/**************************************************************************
/*!
	Link the shared memory map to an hardware pin
	
	It write a digital output pin based on the value of the output into
	memory_map, let a logic act on external devices. Match criteria is based 
	on bit-wise AND operation.
*/	
/**************************************************************************/
void Souliss_nLowDigOut(U8 pin, U8 value, U8 *memory_map, U8 slot)
{
	// If output is active switch on the pin, else off
	if(memory_map[MaCaco_OUT_s + slot] & value)
		digitalWrite(pin, LOW);
	else
		digitalWrite(pin, HIGH);
}

/**************************************************************************
/*!
	Link the shared memory map to an hardware pin, toggle the output value
*/	
/**************************************************************************/
void Souliss_DigOutToggle(U8 pin, U8 value, U8 *memory_map, U8 slot)
{
	if(memory_map[MaCaco_OUT_s + slot] == value)
	{
		// If output is active toggle the pin, else off
		if(digitalRead(pin))
			digitalWrite(pin, LOW);
		else
			digitalWrite(pin, HIGH);
	}
	else
		digitalWrite(pin, LOW);
}

/**************************************************************************
/*!
	Link the shared memory map to an hardware pin
*/	
/**************************************************************************/
void Souliss_DigOutLessThan(U8 pin, U8 value, U8 deadband, U8 *memory_map, U8 slot)
{
	// If output is active switch on the pin, else off
	if(memory_map[MaCaco_OUT_s + slot] < value - deadband)
		digitalWrite(pin, HIGH);
	else if(memory_map[MaCaco_OUT_s + slot] > value + deadband)
		digitalWrite(pin, LOW);
}

/**************************************************************************
/*!
	Link the shared memory map to an hardware pin
*/	
/**************************************************************************/
void Souliss_DigOutGreaterThan(U8 pin, U8 value, U8 deadband, U8 *memory_map, U8 slot)
{
	// If output is active switch on the pin, else off
	if(memory_map[MaCaco_OUT_s + slot] > value + deadband)
			digitalWrite(pin, HIGH);
	else if(memory_map[MaCaco_OUT_s + slot] < value - deadband)
		digitalWrite(pin, LOW);
}

/**************************************************************************
/*!
	This is a special function that can be used to ensure that the output
	command is given just once.
	
	As per MaCaco data structure the OUTPUT slot will remain at last output
	value, this could be a problem for devices that need one shot commands,
	like IR Led Emitter.
	
	In the Typical is specified if this function is required
*/	
/**************************************************************************/
U8 Souliss_isTrigged(U8 *memory_map, U8 slot)
{
	if(memory_map[MaCaco_AUXIN_s + slot] == Souliss_TRIGGED)
	{
		// Reset the trigger and return that trigger was there
		memory_map[MaCaco_AUXIN_s + slot] = Souliss_NOTTRIGGED;
		return Souliss_TRIGGED;
	}	
	
	return Souliss_NOTRIGGED;
}

