#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MD_GalleryCatalog.generated.h"

class AActor;
class UMD_GalleryWidget;

USTRUCT(BlueprintType)
struct MORBUSDEI_API FMDGalleryCatalogEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gallery", meta = (DisplayName = "Asset-Anzeigename", ToolTip = "Der Name unter der Vorschau, z.B. Medizingefäß. Direkt hier eintragen; keine zusätzliche Asset-ID erforderlich. Leer verwendet den Ersatztext des Gallery-Widgets."))
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gallery", meta = (DisplayName = "Artist auswählen", GetOptions = "GetArtistIds", ToolTip = "Wähle einen Schlüssel aus der Liste Artists dieses Data Assets. Mehrere Assets können denselben Artist verwenden. None bedeutet: kein Artist zugeordnet. Den sichtbaren Namen pflegst du einmal zentral unter Artists."))
	FName ArtistId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gallery|Presentation", meta = (DisplayName = "Vorschau-Startrotation", ToolTip = "Lokale Korrektur zur Startrotation des Preview-Rigs. Damit unterschiedliche Import-Achsen zur Kamera ausgerichtet werden. Reset stellt diese Ansicht wieder her."))
	FRotator PreviewRotationOffset = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gallery|Presentation", meta = (DisplayName = "Vorschau-Pivot-Korrektur", Units = "cm", ToolTip = "Optionaler Versatz von der sichtbaren Mesh-Mitte in lokalen Actor-Achsen. Null dreht um die automatisch bestimmte Mitte; bei asymmetrischen Assets hier den gewuenschten Drehpunkt korrigieren."))
	FVector PreviewPivotOffset = FVector::ZeroVector;
};

/** Central, reusable display information for gallery preview classes. */
UCLASS(BlueprintType, meta = (DisplayName = "MD Gallery Catalog", ToolTip = "Namen und Artists für die Gallery zentral bearbeiten. Zuerst Artists anlegen, danach Vorschau-Blueprints unter Gallery-Einträge auswählen und deren Anzeigenamen eintragen. Dieses Data Asset im WBP_Gallery als Gallery-Katalog zuweisen."))
class MORBUSDEI_API UMD_GalleryCatalog : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gallery", meta = (DisplayName = "Artists – zentrale Namensliste", ToolTip = "Links einen selbst gewählten, eindeutigen Schlüssel eintragen (z.B. Artist_Estelle), rechts den sichtbaren Namen (z.B. Estelle). Dieser Schlüssel steht in jedem Gallery-Eintrag als Dropdown bereit. Umbenennen des sichtbaren Namens gilt für alle zugeordneten Assets. None ist kein gültiger Artist-Schlüssel; Schlüssel nach Verwendung nicht ändern."))
	TMap<FName, FText> Artists;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gallery", meta = (DisplayName = "Gallery-Einträge – Vorschau zu Name und Artist", ToolTip = "Links dieselbe Actor-Blueprint-Klasse auswählen, die der Gallery-Button an Show Preview Item / Preview Class übergibt. Rechts Asset-Anzeigename eintragen und Artist auswählen. Kein Static Mesh und kein Vorschaubild. Jede Klasse einmal eintragen; Kindklassen benötigen einen eigenen Eintrag. Referenzen laden die Vorschau-Assets nicht automatisch."))
	TMap<TSoftClassPtr<AActor>, FMDGalleryCatalogEntry> Entries;

	const FMDGalleryCatalogEntry* FindEntry(TSubclassOf<AActor> PreviewClass) const;

	UFUNCTION()
	TArray<FString> GetArtistIds() const;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Category = "Import vorhandener Daten", meta = (DisplayName = "Quell-Widget für Import (optional)", ToolTip = "Optional WBP_Gallery auswählen, wenn dort bereits die alten Credit- und Asset-Name-Maps gefüllt sind. Danach Widget-Zuordnungen übernehmen drücken. Übernommen werden gespeicherte Class Defaults. Das Quell-Widget wird nicht verändert; den Gallery-Katalog danach dort selbst zuweisen."))
	TSoftClassPtr<UMD_GalleryWidget> ImportSourceWidget;
#endif

	UFUNCTION(CallInEditor, Category = "Import vorhandener Daten", meta = (DisplayName = "Widget-Zuordnungen übernehmen", ToolTip = "Liest die bisherigen vier Maps des gewählten Quell-Widgets. Fügt fehlende Einträge hinzu und füllt leere Namen/Artist-Zuordnungen. Bereits ausgefüllte Katalog-Werte bleiben bestehen. Danach dieses Data Asset speichern und im WBP_Gallery als Gallery-Katalog zuweisen."))
	void ImportWidgetMappings();
};
