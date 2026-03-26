# Cook Error Fix: GetSimplePhysicalMaterial During CDO Construction

## Problem
The cook process was failing with:
```
LogPhysics: Error: FBodyInstance::GetSimplePhysicalMaterial : GEngine not initialized! 
Cannot call this during native CDO construction, wrap with if(!HasAnyFlags(RF_ClassDefaultObject)) 
or move out of constructor, material parameters will not be correct.
```

## Root Cause
Multiple classes were calling collision and physics-related functions during Class Default Object (CDO) construction:
- `PUIngredientMesh.cpp`: Called `SetSimulatePhysics`, `SetLinearDamping`, `SetAngularDamping`, `SetMassScale`, `SetCollisionProfileName`, and `SetCollisionEnabled`
- `TalkingObject.cpp`: Called `SetCollisionProfileName` on `InteractionSphere`
- `PUCookingStation.cpp`: Called `SetCollisionProfileName` on `InteractionBox`

These functions (especially `SetCollisionProfileName`) internally call `GetSimplePhysicalMaterial`, which requires `GEngine` to be initialized. During cooking, the CDO is constructed before `GEngine` is available.

## Solution
1. **Removed all collision/physics calls from constructors**: Completely removed `SetCollisionProfileName`, `SetSimulatePhysics`, and other physics-related calls from constructors to prevent CDO construction issues.

2. **Added PostInitializeComponents override**: Added `PostInitializeComponents()` to all affected classes to ensure collision and physics settings are properly applied when the actor is actually spawned in the world, after `GEngine` is initialized.

3. **Applied CDO safety checks**: All `PostInitializeComponents()` implementations check `!HasAnyFlags(RF_ClassDefaultObject)` before applying settings.

## Files Modified
- `Source/ProjectUmeowmi/DishCustomization/PUIngredientMesh.cpp`
- `Source/ProjectUmeowmi/DishCustomization/PUIngredientMesh.h`
- `Source/ProjectUmeowmi/Dialogue/TalkingObject.cpp`
- `Source/ProjectUmeowmi/Dialogue/TalkingObject.h`
- `Source/ProjectUmeowmi/Interactables/PUCookingStation.cpp`
- `Source/ProjectUmeowmi/Interactables/PUCookingStation.h`

## How to Trace Similar Issues

### 1. Search for Physics Calls in Constructors
```bash
# Search for physics-related calls in constructors
grep -r "SetSimulatePhysics\|SetLinearDamping\|SetAngularDamping\|SetMassScale" Source/ --include="*.cpp" -B 5
```

### 2. Look for CDO Construction Issues
The error message will typically mention:
- "GEngine not initialized"
- "CDO construction"
- "HasAnyFlags(RF_ClassDefaultObject)"

### 3. Common Patterns to Watch For
- Any physics component setup in constructors
- Calls to `GetSimplePhysicalMaterial` or `GetPhysicalMaterial`
- Physics property access during object construction
- Material access during CDO construction

### 4. Debugging Approach
1. Check the error log for the specific class mentioned
2. Search for physics-related calls in that class's constructor
3. Wrap problematic calls with: `if (!HasAnyFlags(RF_ClassDefaultObject))`
4. Move initialization to `PostInitializeComponents()` or `BeginPlay()` if needed

### 5. Prevention
- Avoid calling physics functions in constructors that require `GEngine`
- Use `PostInitializeComponents()` for physics setup that needs engine initialization
- Always check `HasAnyFlags(RF_ClassDefaultObject)` before accessing engine-dependent systems in constructors

## Testing
After this fix, the cook process should complete successfully without the `GetSimplePhysicalMaterial` error.

