#include "pch.h"
#include "TLightBargraph.h"


#include "control.h"
#include "loader.h"
#include "memory.h"
#include "objlist_class.h"
#include "timer.h"
#include "TPinballTable.h"

TLightBargraph::TLightBargraph(TPinballTable* table, int groupIndex) : TLightGroup(table, groupIndex)
{
	TimerTimeArray = nullptr;
	TLightBargraph::Reset();
	if (groupIndex > 0)
	{
		float* floatArr = loader::query_float_attribute(groupIndex, 0, 904);
		if (floatArr)
		{
			int count = 2 * List->GetCount();
			TimerTimeArray = reinterpret_cast<float*>(memory::allocate(count * sizeof(float)));
			if (TimerTimeArray)
			{
				for (int i = 0; i < count; ++floatArr)
					TimerTimeArray[i++] = *floatArr;
			}
		}
	}
}

TLightBargraph::~TLightBargraph()
{
	if (TimerTimeArray)
		memory::free(TimerTimeArray);
}

int TLightBargraph::Message(int code, float value)
{
	switch (code)
	{
	case 37:
		return TimeIndex;
	case 45:
		{
			if (TimerBargraph)
			{
				timer::kill(TimerBargraph);
				TimerBargraph = 0;
			}
			auto timeIndex = static_cast<int>(floor(value));
			auto maxCount = 2 * List->GetCount();
			if (timeIndex >= maxCount)
				timeIndex = maxCount - 1;
			if (timeIndex >= 0)
			{
				TLightGroup::Message(45, static_cast<float>(timeIndex / 2));
				if (!(timeIndex & 1))
					TLightGroup::Message(46, 0.0);
				float* timeArray = TimerTimeArray;
				if (timeArray)
					TimerBargraph = timer::set(timeArray[timeIndex], this, BargraphTimerExpired);
				TimeIndex = timeIndex;
			}
			else
			{
				TLightGroup::Message(20, 0.0);
				TimeIndex = 0;
			}
			break;
		}
	case 1011:
		Reset();
		break;
	case 1020:
		if (TimerBargraph)
		{
			timer::kill(TimerBargraph);
			TimerBargraph = 0;
		}
		PlayerTimerIndexBackup[PinballTable->CurrentPlayer] = TimeIndex;
		Reset();
		TimeIndex = PlayerTimerIndexBackup[static_cast<int>(floor(value))];
		if (TimeIndex)
		{
			TLightBargraph::Message(45, static_cast<float>(TimeIndex));
		}
		break;
	case 1024:
		{
			Reset();
			int* playerPtr = PlayerTimerIndexBackup;
			for (auto index = 0; index < PinballTable->PlayerCount; ++index)
			{
				*playerPtr = TimeIndex;

				++playerPtr;
			}
			TLightGroup::Message(1024, value);
			break;
		}
	default:
		TLightGroup::Message(code, value);
		break;
	}
	return 0;
}

void TLightBargraph::Reset()
{
	if (TimerBargraph)
	{
		timer::kill(TimerBargraph);
		TimerBargraph = 0;
	}
	TimeIndex = 0;
	TLightGroup::Reset();
}

void TLightBargraph::BargraphTimerExpired(int timerId, void* caller)
{
	auto bar = static_cast<TLightBargraph*>(caller);
	bar->TimerBargraph = 0;
	if (bar->TimeIndex)
	{
		bar->Message(45, static_cast<float>(bar->TimeIndex - 1));
		control::handler(60, bar);
	}
	else
	{
		bar->Message(20, 0.0);
		control::handler(47, bar);
	}
}
