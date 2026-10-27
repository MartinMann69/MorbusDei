#include "UI/MD_GalleryCatalog.h"

#include "GameFramework/Actor.h"
#include "UI/MD_GalleryWidget.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

DEFINE_LOG_CATEGORY_STATIC(LogMDGalleryCatalog, Log, All);

const FMDGalleryCatalogEntry* UMD_GalleryCatalog::FindEntry(TSubclassOf<AActor> PreviewClass) const
{
	return PreviewClass ? Entries.Find(TSoftClassPtr<AActor>(PreviewClass.Get())) : nullptr;
}

TArray<FString> UMD_GalleryCatalog::GetArtistIds() const
{
	TArray<FString> Options;
	for (const auto& Artist : Artists)
	{
		if (!Artist.Key.IsNone())
		{
			Options.Add(Artist.Key.ToString());
		}
	}
	Options.Sort();
	Options.Insert(FName(NAME_None).ToString(), 0);
	return Options;
}

void UMD_GalleryCatalog::ImportWidgetMappings()
{
#if WITH_EDITOR
	const UClass* SourceClass = ImportSourceWidget.LoadSynchronous();
	if (!SourceClass)
	{
		UE_LOG(LogMDGalleryCatalog, Warning, TEXT("%s: Bitte zuerst ein Quell-Widget fuer den Import auswaehlen."), *GetPathName());
		return;
	}

	const UMD_GalleryWidget* Source = SourceClass->GetDefaultObject<UMD_GalleryWidget>();
	Modify();
	for (const auto& Credit : Source->CreditTexts)
	{
		if (!Credit.Key.IsNone())
		{
			FText& ArtistName = Artists.FindOrAdd(Credit.Key);
			if (ArtistName.IsEmpty())
			{
				ArtistName = Credit.Value;
			}
		}
	}
	for (const auto& Credit : Source->PreviewCreditIds)
	{
		if (Credit.Key)
		{
			FMDGalleryCatalogEntry& Entry = Entries.FindOrAdd(TSoftClassPtr<AActor>(Credit.Key.Get()));
			if (Entry.ArtistId.IsNone())
			{
				Entry.ArtistId = Credit.Value;
			}
		}
	}
	for (const auto& AssetName : Source->PreviewAssetNameIds)
	{
		if (AssetName.Key)
		{
			FMDGalleryCatalogEntry& Entry = Entries.FindOrAdd(TSoftClassPtr<AActor>(AssetName.Key.Get()));
			if (const FText* Text = Source->AssetNameTexts.Find(AssetName.Value); !AssetName.Value.IsNone() && Text && Entry.DisplayName.IsEmpty())
			{
				Entry.DisplayName = *Text;
			}
		}
	}
	MarkPackageDirty();
	UE_LOG(LogMDGalleryCatalog, Log, TEXT("%s: Import aus %s abgeschlossen. %d Eintraege, %d Artists. Katalog speichern und im Gallery-Widget zuweisen."), *GetPathName(), *SourceClass->GetPathName(), Entries.Num(), Artists.Num());
#endif
}

#if WITH_EDITOR
EDataValidationResult UMD_GalleryCatalog::IsDataValid(FDataValidationContext& Context) const
{
	const EDataValidationResult SuperResult = Super::IsDataValid(Context);
	bool bValid = SuperResult != EDataValidationResult::Invalid;
	for (const auto& Artist : Artists)
	{
		if (Artist.Key.IsNone() || Artist.Value.IsEmpty())
		{
			Context.AddError(FText::Format(NSLOCTEXT("MDGallery", "InvalidArtist", "Artist '{0}': Schlüssel darf nicht None sein und Anzeigename darf nicht leer sein."), FText::FromName(Artist.Key)));
			bValid = false;
		}
	}
	for (const auto& Item : Entries)
	{
		if (Item.Key.IsNull())
		{
			Context.AddError(NSLOCTEXT("MDGallery", "MissingPreviewClass", "Gallery-Eintrag ohne Vorschau-Blueprint. Bitte links eine Actor-Blueprint-Klasse auswählen."));
			bValid = false;
		}
		if (!Item.Value.ArtistId.IsNone() && !Artists.Contains(Item.Value.ArtistId))
		{
			Context.AddError(FText::Format(NSLOCTEXT("MDGallery", "UnknownArtist", "Vorschau '{0}': Artist '{1}' fehlt in der zentralen Artists-Liste."), FText::FromString(Item.Key.ToString()), FText::FromName(Item.Value.ArtistId)));
			bValid = false;
		}
		if (Item.Value.DisplayName.IsEmpty())
		{
			Context.AddWarning(FText::Format(NSLOCTEXT("MDGallery", "MissingDisplayName", "Vorschau '{0}' hat keinen Asset-Anzeigenamen; das Widget verwendet den Ersatztext."), FText::FromString(Item.Key.ToString())));
		}
	}
	return bValid ? EDataValidationResult::Valid : EDataValidationResult::Invalid;
}
#endif
