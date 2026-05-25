#include "PUPreparationBase.h"
#include "GameplayTagsManager.h"
#include "Engine/Engine.h"

FName FAspectModifier::GetAspectName() const
{
    if (AspectType == EAspectType::Flavor)
    {
        return PUAspectHelpers::FlavorAspectToName(FlavorAspect);
    }
    return PUAspectHelpers::TextureAspectToName(TextureAspect);
}

FPUPreparationBase::FPUPreparationBase()
    : DisplayName(FText::GetEmpty())
    , Description(FText::GetEmpty())
    , IconTexture(nullptr)
    , PrepTexture(nullptr)
    , NamePrefix(FText::GetEmpty())
    , NameSuffix(FText::GetEmpty())
    , OverridesBaseName(false)
    , SpecialName(FText::GetEmpty())
{
}

bool FPUPreparationBase::CanApplyToIngredient(const FGameplayTagContainer& IngredientTags) const
{
    // Check if ingredient has incompatible tags
    if (IngredientTags.HasAny(IncompatibleTags))
    {
        return false;
    }

    // Check if ingredient has required tags
    if (!IngredientTags.HasAll(RequiredTags))
    {
        return false;
    }

    return true;
}

FText FPUPreparationBase::GetModifiedName(const FText& BaseName) const
{
    if (OverridesBaseName)
    {
        return SpecialName;
    }

    FString Result;
    if (!NamePrefix.IsEmpty())
    {
        Result += NamePrefix.ToString() + " ";
    }
    
    Result += BaseName.ToString();
    
    if (!NameSuffix.IsEmpty())
    {
        Result += " " + NameSuffix.ToString();
    }

    return FText::FromString(Result);
}

void FPUPreparationBase::ApplyModifiers(FFlavorAspects& FlavorAspects, FTextureAspects& TextureAspects) const
{
    for (const FAspectModifier& Modifier : AspectModifiers)
    {
        FName AspectName = Modifier.GetAspectName();
        FString AspectStr = AspectName.ToString().ToLower();
        float OldVal = 0.0f;
        float NewVal = 0.0f;
        bool bApplied = false;
        
        if (Modifier.AspectType == EAspectType::Flavor)
        {
            if (AspectStr == TEXT("umami")) { OldVal = FlavorAspects.Umami; FlavorAspects.Umami = Modifier.ApplyModification(FlavorAspects.Umami); NewVal = FlavorAspects.Umami; bApplied = true; }
            else if (AspectStr == TEXT("sweet")) { OldVal = FlavorAspects.Sweet; FlavorAspects.Sweet = Modifier.ApplyModification(FlavorAspects.Sweet); NewVal = FlavorAspects.Sweet; bApplied = true; }
            else if (AspectStr == TEXT("salt")) { OldVal = FlavorAspects.Salt; FlavorAspects.Salt = Modifier.ApplyModification(FlavorAspects.Salt); NewVal = FlavorAspects.Salt; bApplied = true; }
            else if (AspectStr == TEXT("sour")) { OldVal = FlavorAspects.Sour; FlavorAspects.Sour = Modifier.ApplyModification(FlavorAspects.Sour); NewVal = FlavorAspects.Sour; bApplied = true; }
            else if (AspectStr == TEXT("bitter")) { OldVal = FlavorAspects.Bitter; FlavorAspects.Bitter = Modifier.ApplyModification(FlavorAspects.Bitter); NewVal = FlavorAspects.Bitter; bApplied = true; }
            else if (AspectStr == TEXT("spicy")) { OldVal = FlavorAspects.Spicy; FlavorAspects.Spicy = Modifier.ApplyModification(FlavorAspects.Spicy); NewVal = FlavorAspects.Spicy; bApplied = true; }
        }
        else // EAspectType::Texture
        {
            if (AspectStr == TEXT("rich")) { OldVal = TextureAspects.Rich; TextureAspects.Rich = Modifier.ApplyModification(TextureAspects.Rich); NewVal = TextureAspects.Rich; bApplied = true; }
            else if (AspectStr == TEXT("juicy")) { OldVal = TextureAspects.Juicy; TextureAspects.Juicy = Modifier.ApplyModification(TextureAspects.Juicy); NewVal = TextureAspects.Juicy; bApplied = true; }
            else if (AspectStr == TEXT("tender")) { OldVal = TextureAspects.Tender; TextureAspects.Tender = Modifier.ApplyModification(TextureAspects.Tender); NewVal = TextureAspects.Tender; bApplied = true; }
            else if (AspectStr == TEXT("chewy")) { OldVal = TextureAspects.Chewy; TextureAspects.Chewy = Modifier.ApplyModification(TextureAspects.Chewy); NewVal = TextureAspects.Chewy; bApplied = true; }
            else if (AspectStr == TEXT("crispy")) { OldVal = TextureAspects.Crispy; TextureAspects.Crispy = Modifier.ApplyModification(TextureAspects.Crispy); NewVal = TextureAspects.Crispy; bApplied = true; }
            else if (AspectStr == TEXT("crumbly")) { OldVal = TextureAspects.Crumbly; TextureAspects.Crumbly = Modifier.ApplyModification(TextureAspects.Crumbly); NewVal = TextureAspects.Crumbly; bApplied = true; }
        }
        
        if (bApplied)
        {
            UE_LOG(LogTemp, Warning, TEXT("[Prep] ApplyModifiers: %s %.2f -> %.2f (mod value %.2f, %s)"),
                *AspectName.ToString(), OldVal, NewVal, Modifier.ModificationValue, Modifier.ModificationType == EModificationType::Additive ? TEXT("additive") : TEXT("multiplicative"));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[Prep] ApplyModifiers: Unknown aspect '%s' (type %s) - modifier NOT applied"),
                *AspectStr, Modifier.AspectType == EAspectType::Flavor ? TEXT("Flavor") : TEXT("Texture"));
        }
    }
}

void FPUPreparationBase::RemoveModifiers(FFlavorAspects& FlavorAspects, FTextureAspects& TextureAspects) const
{
    for (const FAspectModifier& Modifier : AspectModifiers)
    {
        FName AspectName = Modifier.GetAspectName();
        FString AspectStr = AspectName.ToString().ToLower();
        
        if (Modifier.AspectType == EAspectType::Flavor)
        {
            if (AspectStr == TEXT("umami"))
                FlavorAspects.Umami = Modifier.RemoveModification(FlavorAspects.Umami);
            else if (AspectStr == TEXT("sweet"))
                FlavorAspects.Sweet = Modifier.RemoveModification(FlavorAspects.Sweet);
            else if (AspectStr == TEXT("salt"))
                FlavorAspects.Salt = Modifier.RemoveModification(FlavorAspects.Salt);
            else if (AspectStr == TEXT("sour"))
                FlavorAspects.Sour = Modifier.RemoveModification(FlavorAspects.Sour);
            else if (AspectStr == TEXT("bitter"))
                FlavorAspects.Bitter = Modifier.RemoveModification(FlavorAspects.Bitter);
            else if (AspectStr == TEXT("spicy"))
                FlavorAspects.Spicy = Modifier.RemoveModification(FlavorAspects.Spicy);
        }
        else // EAspectType::Texture
        {
            if (AspectStr == TEXT("rich"))
                TextureAspects.Rich = Modifier.RemoveModification(TextureAspects.Rich);
            else if (AspectStr == TEXT("juicy"))
                TextureAspects.Juicy = Modifier.RemoveModification(TextureAspects.Juicy);
            else if (AspectStr == TEXT("tender"))
                TextureAspects.Tender = Modifier.RemoveModification(TextureAspects.Tender);
            else if (AspectStr == TEXT("chewy"))
                TextureAspects.Chewy = Modifier.RemoveModification(TextureAspects.Chewy);
            else if (AspectStr == TEXT("crispy"))
                TextureAspects.Crispy = Modifier.RemoveModification(TextureAspects.Crispy);
            else if (AspectStr == TEXT("crumbly"))
                TextureAspects.Crumbly = Modifier.RemoveModification(TextureAspects.Crumbly);
        }
    }
} 