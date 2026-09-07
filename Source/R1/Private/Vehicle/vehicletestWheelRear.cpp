


#include "Vehicle/vehicletestWheelRear.h"

UvehicletestWheelRear::UvehicletestWheelRear()
{
	AxleType = EAxleType::Rear;
	bAffectedByHandbrake = true;
	bAffectedByEngine = true;

	WheelRadius = 55.f;
}
