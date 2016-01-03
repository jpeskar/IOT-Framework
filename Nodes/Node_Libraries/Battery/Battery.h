#ifndef Battery_h
#endif  Battery_h


class Battery  {
	public:
		
		float	 BatteryMeasurement();
		
	private:
		uint16_t ReadVcc();
	};
extern Battery battery;
#endif