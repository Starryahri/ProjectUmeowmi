#include "PUUObjectSafety.h"

#include "PUDishCustomizationWidget.h"
#include "PUIngredientSlot.h"
#include "PURadarChart.h"
#include "PURadialMenu.h"
#include "UObject/GarbageCollection.h"
#include "UObject/UObjectIterator.h"

#if PLATFORM_WINDOWS
#include "Windows/AllowWindowsPlatformTypes.h"
#include <excpt.h>
#include "Windows/HideWindowsPlatformTypes.h"
#endif

namespace PUObjectReferenceSafety
{
	bool CanQueryUObject(const UObject* Obj)
	{
		if (Obj == nullptr)
		{
			return false;
		}

		const UPTRINT Addr = reinterpret_cast<UPTRINT>(Obj);
		if (Addr < 0x10000)
		{
			return false;
		}

#if PLATFORM_WINDOWS
		__try
		{
			return Obj->IsValidLowLevelFast(false);
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			return false;
		}
#else
		return Obj->IsValidLowLevelFast(false);
#endif
	}

	bool IsLiveObject(const UObject* Obj)
	{
		if (!CanQueryUObject(Obj))
		{
			return false;
		}

		return IsValid(Obj);
	}

	void SanitizeAllCustomizationUIObjectReferencesBeforeGC()
	{
		if (IsGarbageCollecting())
		{
			return;
		}

		UPURadarChart::SanitizeAllLiveRadarCharts();
		UPURadialMenu::SanitizeAllLiveRadialMenus();
		UPUIngredientSlot::SanitizeAllLiveIngredientSlots();
		UPUDishCustomizationWidget::SanitizeAllLiveDishCustomizationWidgets();
	}

	void RegisterCustomizationUIPreGarbageCollectSanitizer()
	{
		// Intentionally empty — pre-GC UObject iteration can interfere with incremental GC.
	}

	void UnregisterCustomizationUIPreGarbageCollectSanitizer()
	{
	}
}
