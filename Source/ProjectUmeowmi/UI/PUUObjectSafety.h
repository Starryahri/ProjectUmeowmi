#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

/** Defensive UObject pointer checks (handles wild / freed pointers). */
namespace PUObjectReferenceSafety
{
	PROJECTUMEOWMI_API bool CanQueryUObject(const UObject* Obj);
	PROJECTUMEOWMI_API bool IsLiveObject(const UObject* Obj);

	/** Clears stale UObject* in radar segments, radial menu items, slots, and dish widget arrays before GC. */
	PROJECTUMEOWMI_API void SanitizeAllCustomizationUIObjectReferencesBeforeGC();

	PROJECTUMEOWMI_API void RegisterCustomizationUIPreGarbageCollectSanitizer();
	PROJECTUMEOWMI_API void UnregisterCustomizationUIPreGarbageCollectSanitizer();
}
