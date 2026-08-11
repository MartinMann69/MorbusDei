#include "Haptics/MD_HapticBlueprintLibrary.h"

#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Haptics/MD_HapticSubsystem.h"

bool UMD_HapticBlueprintLibrary::PlayMDHapticEvent(
	const UObject* WorldContextObject,
	const EMDHapticEvent Event)
{
	const UWorld* World = GEngine
		? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull)
		: nullptr;
	APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
	ULocalPlayer* LocalPlayer = PlayerController ? PlayerController->GetLocalPlayer() : nullptr;
	UMD_HapticSubsystem* HapticSubsystem = LocalPlayer
		? LocalPlayer->GetSubsystem<UMD_HapticSubsystem>()
		: nullptr;

	return HapticSubsystem && HapticSubsystem->PlayHapticEvent(Event);
}
