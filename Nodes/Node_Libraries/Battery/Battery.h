#ifndef Battery_h
#define Battery_h


class Battery  {
	public:
		
		float	 BatteryMeasurement(int pin);
		int ReadVcc();
			
	};
extern Battery battery;
#endif