#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Haptics/MD_HapticTypes.h"
#include "MD_HapticTriggerVolume.generated.h"

class UBoxComponent;
class UPrimitiveComponent;

/** Designer-placed, independent haptic story beat with no runtime tick. */
UCLASS(Blueprintable)
class MORBUSDEI_API AMD_HapticTriggerVolume : public AActor
{
	GENERATED_BODY()

public:
	AMD_HapticTriggerVolume();

	virtual void BeginPlay() override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "MD|Haptics|Components")
	TObjectPtr<UBoxComponent> Trigger;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MD|Haptics")
	EMDHapticEvent Event = EMDHapticEvent::StoryLight;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MD|Haptics")
	bool bTriggerOnce = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MD|Haptics",
		meta = (ClampMin = "0.0", EditCondition = "!bTriggerOnce", Units = "s"))
	float RetriggerCooldown = 0.5f;

private:
	UFUNCTION()
	void HandleBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComponent,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	double LastSuccessfulTriggerRealTime = -DBL_MAX;
};
