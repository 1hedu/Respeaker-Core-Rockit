char readSwitch(void);

#define SWITCH_IS_PRESSED !(CHECKBIT(PIND, PD7))


char readSwitch(void)
{
	static unsigned short ucdebounceTimer = 8; 	

	//if the button is down and stays down, the timer will run down to zero
	//otherwise, the timer gets reset
	if(SWITCH_IS_PRESSED)
	{
		
		if(ucdebounceTimer > 0)
		{
			ucdebounceTimer--;
		} 
		else
		{
			return 1;
		}
	
	}	
	else
	{
		ucdebounceTimer = 500;//about 25ms is a good time for debouncing
		
	}


	return 0;
}
