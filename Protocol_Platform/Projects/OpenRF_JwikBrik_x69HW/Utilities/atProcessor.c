/*************************************************************************************
**																					**
**	atProcessor.c		Si1000 processor (8051 derivative							**
** 																					**
**************************************************************************************
**																					**
** Written By:	Steve Montgomery													**
**				Digital Six Laboratories LLC										**
** (c)2012 Digital Six Labs, All rights reserved									**
**																					**
**************************************************************************************/
//
// Revision History
//
// Revision		Date	Revisor		Description
// ===================================================================================
// ===================================================================================


#include "..\..\..\SourceCode\Radio\Sx1231\radioapi.h"
#include "..\..\..\SourceCode\OpenRF_MAC\openrf_mac.h"
#include "atProcessor.h"



#define kATBufferSize 40
tAtStates _atState;
tCallback _commandCallback;
U8 xdata atBuffer[kATBufferSize];
U8 xdata bufPtr = 0;
U8 xdata foundA = 0;
U8 xdata foundT = 0;
U8 xdata _commandCount;
U8 xdata _maxCommandSize;
U8 _bufCount = 0;
U8 *_commands[32];

void ProcessATCommand();
tAtStates ATGetState()
{
	return _atState;
}
void ATInitialize(U8 *commands[], U8 commandCount, tCallback callback)
{
	U8 i;
	_atState = kDisabled;
	bufPtr=0;
	_bufCount = 0;
	foundA=0;
	foundT = 0;
	for(i=0;i<commandCount;i++)
	{
		_commands[i] = commands[i];
	}
	_commandCount = commandCount;
	_commandCallback = callback;
}
void ATProcess()
{
	U8 ch;
	switch(_atState)
	{
		case kDisabled:
			if(BufferCountUART1()>0)
			{
				ch=Uart1PeekByte();
				if(ch=='+')
				{
					_atState=kPlus1;
					ReadCharUART1();
				}
			}
			break;
		case kPlus1:
			if(BufferCountUART1()>0)
			{
				ch=Uart1PeekByte();
				if(ch=='+')
				{
					_atState=kPlus2;
					ReadCharUART1();
				}
				else
					_atState=kDisabled;
			}
			break;
		case kPlus2:
			if(BufferCountUART1()>0)
			{

				ch=Uart1PeekByte();
				if(ch=='+')
				{
					ReadCharUART1();
					_atState=kEnabled;
					WriteCharUART1('O');
					WriteCharUART1('K');
					WriteCharUART1(0x0a);
					WriteCharUART1(0x0d);
				}
				else
					_atState=kDisabled;
			}
			break;
		case kEnabled:
			ProcessATCommand();
			break;
	}
}
U8 Compare(U8 *cmdToCompare)
{
	U8 bp=0;
	U8 cnt=0;
	U8 *ptr = cmdToCompare;
	// cycle through string and compare to buffer
	while(*ptr!=0)
	{
		if(*ptr++ != atBuffer[bp++])
			return 0;
		cnt++;
	}
	// if we found our command, we need to update bufPtr so it points to the next spot in the buffer after the last command character
	bufPtr = bp;
	return cnt;
}
void ProcessATCommand()
{
	U8 ch;
	U8 count;
	U8 i;
	// first, load all the characters we can into the buffer
	//
	count = BufferCountUART1();
	while(count--)
	{
		ch=ReadCharUART1();
		if(!foundA)
		{
			if(ch=='A')
			{
				foundA=1;
			}
		}
		else if(!foundT)
		{
			if(ch=='T')
			{
				foundT=1;
				bufPtr=0;
			}
		}
		else
		{

			if((ch==0x0a) || (ch==0x0d))
			{
				_bufCount = bufPtr;
				if(bufPtr==0)
				{
					// return the null command
					_commandCallback(0xff);

				}
				else
				{
					// in here, we have received the terminal character so we need to see if we can process the command.
					for(i=0;i<_commandCount;i++)
					{
						count = Compare(_commands[i]);
						if(count)
						{
							_commandCallback(i);
							i=_commandCount;
						}
					}
				}
				foundA = 0;
				foundT = 0;
				bufPtr = 0;
				_bufCount=0;
			}
			else
			{
				// only put the char in the buffer if the buffer is not overflowing.  Otherwise, ignore it unless it is a terminal character

				if(bufPtr<kATBufferSize)
					atBuffer[bufPtr++]=ch;
			}
		}
	}
}
U8 IsATBufferNotEmpty()
{
	if(_bufCount>bufPtr)
		return 1;

	return 0;
}
U8 GetATBufferCharacter()
{
	return atBuffer[bufPtr++];
}
void ExitCommandMode()
{
	_atState = kDisabled;
}
U8 IsInCommandMode()
{
	return _atState!=kDisabled;
}
