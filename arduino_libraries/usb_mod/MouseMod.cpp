
/*
  Mouse.cpp

  Copyright (c) 2015, Arduino LLC
  Original code (pre-library): Copyright (c) 2011, Peter Barrett

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "MouseMod.h"

#if defined(_USING_DYNAMIC_HID)

static const uint8_t _hidReportDescriptor[] PROGMEM = {
  
  //  Mouse
    0x05, 0x01,                    // USAGE_PAGE (Generic Desktop)  // 54
    0x09, 0x02,                    // USAGE (Mouse)
    0xa1, 0x01,                    // COLLECTION (Application)
    0x09, 0x01,                    //   USAGE (Pointer)
    0xa1, 0x00,                    //   COLLECTION (Physical)
    0x85, 0x01,                    //     REPORT_ID (1)
    0x05, 0x09,                    //     USAGE_PAGE (Button)
    0x19, 0x01,                    //     USAGE_MINIMUM (Button 1)
    0x29, 0x03,                    //     USAGE_MAXIMUM (Button 3)
    0x15, 0x00,                    //     LOGICAL_MINIMUM (0)
    0x25, 0x01,                    //     LOGICAL_MAXIMUM (1)
    0x95, 0x03,                    //     REPORT_COUNT (3)
    0x75, 0x01,                    //     REPORT_SIZE (1)
    0x81, 0x02,                    //     INPUT (Data,Var,Abs)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x75, 0x05,                    //     REPORT_SIZE (5)
    0x81, 0x03,                    //     INPUT (Cnst,Var,Abs)
    0x05, 0x01,                    //     USAGE_PAGE (Generic Desktop)
    0x09, 0x30,                    //     USAGE (X)
    0x09, 0x31,                    //     USAGE (Y)
    0x09, 0x38,                    //     USAGE (Wheel)
    0x15, 0x81,                    //     LOGICAL_MINIMUM (-127)
    0x25, 0x7f,                    //     LOGICAL_MAXIMUM (127)
    0x75, 0x08,                    //     REPORT_SIZE (8)
    0x95, 0x03,                    //     REPORT_COUNT (3)
    0x81, 0x06,                    //     INPUT (Data,Var,Rel)
//
    0x05, 0x0c,                    //     USAGE PAGE (Consumer Devices)
    0x0a, 0x38, 0x02,              //     USAGE (AC Pan)
    0x15, 0x81,                    //     LOGICAL_MINIMUM (-127)
    0x25, 0x7f,                    //     LOGICAL_MAXIMUM (127)
    0x75, 0x08,                    //     REPORT_SIZE (8)
    0x95, 0x01,                    //     REPORT_COUNT (1)
    0x81, 0x06,                    //     INPUT (Data,Var,Rel)
//
    0xc0,                          //   END_COLLECTION
    0xc0,                          // END_COLLECTION
};

//================================================================================
//================================================================================
//	Mouse

MouseMod_::MouseMod_(void) : _buttons(0)
{
    static DynamicHIDSubDescriptor node(_hidReportDescriptor, sizeof(_hidReportDescriptor));
    DynamicHID().AppendDescriptor(&node);
}

void MouseMod_::begin(void) 
{
}

void MouseMod_::end(void) 
{
}

void MouseMod_::click(uint8_t b)
{
	_buttons = b;
	move(0,0,0);
	_buttons = 0;
	move(0,0,0);
}

void MouseMod_::move(signed char x, signed char y, signed char wheel,
        signed char wheel_h)
{
	uint8_t m[5];
	m[0] = _buttons;
	m[1] = x;
	m[2] = y;
	m[3] = wheel;
	m[4] = wheel_h;
	DynamicHID().SendReport(1,m,5);
}

void MouseMod_::report(uint8_t b, signed char x, signed char y,
        signed char wheel, signed char wheel_h)
{
	uint8_t m[5];
        _buttons = b;
	m[0] = _buttons;
	m[1] = x;
	m[2] = y;
	m[3] = wheel;
	m[4] = wheel_h;
	DynamicHID().SendReport(1,m,5);
}

void MouseMod_::buttons(uint8_t b)
{
	if (b != _buttons)
	{
		_buttons = b;
		move(0,0,0);
	}
}

void MouseMod_::press(uint8_t b) 
{
	buttons(_buttons | b);
}

void MouseMod_::release(uint8_t b)
{
	buttons(_buttons & ~b);
}

bool MouseMod_::isPressed(uint8_t b)
{
	if ((b & _buttons) > 0) 
		return true;
	return false;
}

// MouseMod_ MouseMod;

#endif

