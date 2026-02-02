#include "PUPreparationBase.h"
#include "GameplayTagsManager.h"

FPUPreparationBase::FPUPreparationBase()
    : DisplayName(FText::GetEmpty())
    , Description(FText::GetEmpty())
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
        FName AspectName = Modifier.AspectName;
        FString AspectStr = AspectName.ToString().ToLower();
        
        if (Modifier.AspectType == EAspectType::Flavor)
        {
            if (AspectStr == TEXT("umami"))
                FlavorAspects.Umami = Modifier.ApplyModification(FlavorAspects.Umami);
            else if (AspectStr == TEXT("sweet"))
                FlavorAspects.Sweet = Modifier.ApplyModification(FlavorAspects.Sweet);
            else if (AspectStr == TEXT("salt"))
                FlavorAspects.Salt = Modifier.ApplyModification(FlavorAspects.Salt);
            else if (AspectStr == TEXT("sour"))
                FlavorAspects.Sour = Modifier.ApplyModification(FlavorAspects.Sour);
            else if (AspectStr == TEXT("bitter"))
                FlavorAspects.Bitter = Modifier.ApplyModification(FlavorAspects.Bitter);
            else if (AspectStr == TEXT("spicy"))
                FlavorAspects.Spicy = Modifier.ApplyModification(FlavorAspects.Spicy);
        }
        else // EAspectType::Texture
        {
            if (AspectStr == TEXT("rich"))
                TextureAspects.Rich = Modifier.ApplyModification(TextureAspects.Rich);
            else if (AspectStr == TEXT("juicy"))
                TextureAspects.Juicy = Modifier.ApplyModification(TextureAspects.Juicy);
            else if (AspectStr == TEXT("tender"))
                TextureAspects.Tender = Modifier.ApplyModification(TextureAspects.Tender);
            else if (AspectStr == TEXT("chewy"))
                TextureAspects.Chewy = Modifier.ApplyModification(TextureAspects.Chewy);
            else if (AspectStr == TEXT("crispy"))
                TextureAspects.Crispy = Modifier.ApplyModification(TextureAspects.Crispy);
            else if (AspectStr == TEXT("crumbly"))
                TextureAspects.Crumbly = Modifier.ApplyModification(TextureAspects.Crumbly);
        }
    }
}

void FPUPreparationBase::RemoveModifiers(FFlavorAspects& FlavorAspects, FTextureAspects& TextureAspects) const
{
    for (const FAspectModifier& Modifier : AspectModifiers)
    {
        FName AspectName = Modifier.AspectName;
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