using UnityEngine;
using UnityEngine.SceneManagement;

public static class RandomItemGenerator
{
    static string[] prefixes = { "", "Damaged ", "Great ", "Bad ", "Prysmatic " , "Legacy ", "Overdue "};
    static string[] suffixes = { "", " of Glory", " of Madness", " of Disgrace", " of Honor" , " on Steroids", " bought on sale"};
    static int numberOfItemTypes = System.Enum.GetNames(typeof(ItemType)).Length;

    private static ItemRarity GenerateRarity()
    {
        float rarity = Random.Range(0f, 1f);
        if (rarity < .01f)
        {
            return ItemRarity.Legendary;
        }
        if (rarity < .05f)
        {
            return ItemRarity.Epic;
        }    
        if (rarity < .1f)
        {
            return ItemRarity.Rare;
        }
        if (rarity < .25f)
        {
            return ItemRarity.Uncommon;
        }
        return ItemRarity.Common;
    }

    private static string CreatePrefix()
    {
        return prefixes[Random.Range(0, prefixes.Length)];
    }

    private static string CreateSuffix()
    {
        return suffixes[Random.Range(0, suffixes.Length)]; ;
    }
    
    private static ScriptableItem GenearateScriptableWeapon()
    {
        ScriptableItem scriptableWeapon = ScriptableObject.CreateInstance<ScriptableItem>();
        scriptableWeapon.Type = ItemType.Weapon;
        scriptableWeapon.additionalAttributes.Add(ItemAdditionalAttributes.MinDamage, Random.Range(4, 20));
        scriptableWeapon.additionalAttributes.Add(ItemAdditionalAttributes.MaxDamage, Random.Range((int)scriptableWeapon.additionalAttributes[ItemAdditionalAttributes.MinDamage], 2 * (int)scriptableWeapon.additionalAttributes[ItemAdditionalAttributes.MinDamage]));
        scriptableWeapon.additionalAttributes.Add(ItemAdditionalAttributes.AttackRate, Random.Range(1, 10));
        scriptableWeapon.additionalAttributes.Add(ItemAdditionalAttributes.Range, Random.Range(1, 10));
        scriptableWeapon.additionalAttributes.Add(ItemAdditionalAttributes.Parry, Random.Range(0, 100));
        scriptableWeapon.additionalAttributes.Add(ItemAdditionalAttributes.UseInPlace, 1);
        scriptableWeapon.additionalAttributes.Add(ItemAdditionalAttributes.OnUseDamage, Random.Range(20, 40));
        scriptableWeapon.additionalAttributes.Add(ItemAdditionalAttributes.OnUseCooldown, Random.Range(1, 10));
        scriptableWeapon.additionalAttributes.Add(ItemAdditionalAttributes.OnUseAttack, 1);
        scriptableWeapon.Rarity = GenerateRarity();
        scriptableWeapon.itemName = scriptableWeapon.Rarity + " " + CreatePrefix() + scriptableWeapon.Type.ToString() + CreateSuffix();

        scriptableWeapon.sprite = "Inventory/Sprites/" + scriptableWeapon.Type + "/" + Random.Range(0, 9);
        return scriptableWeapon;
    }
    private static ScriptableItem GenearateScriptableRangedWeapon()
    {
        ScriptableItem scriptableWeapon = ScriptableObject.CreateInstance<ScriptableItem>();
        scriptableWeapon.Type = ItemType.RangedWeapon;
        scriptableWeapon.additionalAttributes.Add(ItemAdditionalAttributes.MinDamage, Random.Range(4, 20));
        scriptableWeapon.additionalAttributes.Add(ItemAdditionalAttributes.MaxDamage, Random.Range((int)scriptableWeapon.additionalAttributes[ItemAdditionalAttributes.MinDamage], 2 * (int)scriptableWeapon.additionalAttributes[ItemAdditionalAttributes.MinDamage]));
        scriptableWeapon.additionalAttributes.Add(ItemAdditionalAttributes.AttackRate, Random.Range(1, 10));
        scriptableWeapon.additionalAttributes.Add(ItemAdditionalAttributes.Range, Random.Range(5, 20));
        scriptableWeapon.additionalAttributes.Add(ItemAdditionalAttributes.Parry, Random.Range(0, 100));
        scriptableWeapon.additionalAttributes.Add(ItemAdditionalAttributes.UseShotAtTarget, 1);
        scriptableWeapon.additionalAttributes.Add(ItemAdditionalAttributes.OnUseDamage, Random.Range(20, 40));
        scriptableWeapon.additionalAttributes.Add(ItemAdditionalAttributes.OnUseCooldown, Random.Range(1, 10));
        scriptableWeapon.additionalAttributes.Add(ItemAdditionalAttributes.OnUseAttack, 1);
        scriptableWeapon.sprite = "Inventory/Sprites/" + scriptableWeapon.Type + "/" + Random.Range(0, 2);
        scriptableWeapon.Rarity = GenerateRarity();
        scriptableWeapon.itemName = scriptableWeapon.Rarity + " " + CreatePrefix() + "Bow" + CreateSuffix();
        return scriptableWeapon;
    }

    private static ScriptableItem GenearateScriptableHelmet()
    {
        ScriptableItem scriptableHelmet = ScriptableObject.CreateInstance<ScriptableItem>();
        scriptableHelmet.Type = ItemType.Helmet;
        scriptableHelmet.additionalAttributes.Add(ItemAdditionalAttributes.Armor, Random.Range(1, 10));
        scriptableHelmet.additionalAttributes.Add(ItemAdditionalAttributes.DamageReduction, Random.Range(1, 10));
        scriptableHelmet.additionalAttributes.Add(ItemAdditionalAttributes.Durability, Random.Range(1, 10));
        scriptableHelmet.additionalAttributes.Add(ItemAdditionalAttributes.DamageIgnore, Random.Range(1, 10));
        scriptableHelmet.sprite = "Inventory/Sprites/" + scriptableHelmet.Type + "/" + Random.Range(0, 6);
        scriptableHelmet.Rarity = GenerateRarity();
        scriptableHelmet.itemName = scriptableHelmet.Rarity + " " + CreatePrefix() + scriptableHelmet.Type.ToString() + CreateSuffix();
        return scriptableHelmet;
    }

    private static ScriptableItem GenearateScriptableShield()
    {
        ScriptableItem scriptableShield = ScriptableObject.CreateInstance<ScriptableItem>();
        scriptableShield.Type = ItemType.Shield;
        scriptableShield.additionalAttributes.Add(ItemAdditionalAttributes.Armor, Random.Range(1, 10));
        scriptableShield.additionalAttributes.Add(ItemAdditionalAttributes.DamageReduction, Random.Range(1, 10));
        scriptableShield.additionalAttributes.Add(ItemAdditionalAttributes.Durability, Random.Range(1, 10));
        scriptableShield.additionalAttributes.Add(ItemAdditionalAttributes.DamageIgnore, Random.Range(1, 10));
        scriptableShield.sprite = "Inventory/Sprites/" + scriptableShield.Type + "/" + Random.Range(0, 3);
        scriptableShield.Rarity = GenerateRarity();
        scriptableShield.itemName = scriptableShield.Rarity + " " + CreatePrefix() + scriptableShield.Type.ToString() + CreateSuffix();
        return scriptableShield;
    }
    private static ScriptableItem GenearateScriptableArtifact()
    {
        ScriptableItem scriptableArtifact = ScriptableObject.CreateInstance<ScriptableItem>();
        scriptableArtifact.Type = ItemType.Artifact;
        scriptableArtifact.Rarity = ItemRarity.Legendary;
        scriptableArtifact.sprite = "Inventory/Sprites/" + scriptableArtifact.Type + "/" + SceneManager.GetActiveScene().name + "/" + Random.Range(0, 2);
        scriptableArtifact.itemName = CreatePrefix() + scriptableArtifact.Type.ToString() + CreateSuffix();

        scriptableArtifact.additionalAttributes.Add(ItemAdditionalAttributes.UseInPlace, 1);
        scriptableArtifact.additionalAttributes.Add(ItemAdditionalAttributes.AreaOfEffectRadius, Random.Range(1, 6));
        scriptableArtifact.additionalAttributes.Add(ItemAdditionalAttributes.OnUseFreeze, Random.Range(1, 4));
        scriptableArtifact.additionalAttributes.Add(ItemAdditionalAttributes.OnUseCooldown, Random.Range(1, 4));

        return scriptableArtifact;
    }

    private static ScriptableItem GenearateScriptableNormal()
    {
        ScriptableItem scriptableNormal = ScriptableObject.CreateInstance<ScriptableItem>();
        scriptableNormal.Type = ItemType.Normal;
        scriptableNormal.sprite = "Inventory/Sprites/" + scriptableNormal.Type + "/" + Random.Range(0, 19);
        scriptableNormal.Rarity = GenerateRarity();
        scriptableNormal.itemName = scriptableNormal.Rarity + " " + CreatePrefix() + "Item" + CreateSuffix();
        return scriptableNormal;
    }
    private static ScriptableItem GenearateScriptableHealthPotion()
    {
        ScriptableItem scriptableHealthPotion = ScriptableObject.CreateInstance<ScriptableItem>();
        scriptableHealthPotion.stackable = true;
        scriptableHealthPotion.Type = ItemType.HealthPotion;
        scriptableHealthPotion.sprite = "Inventory/Sprites/" + scriptableHealthPotion.Type + "/" + 1;
        scriptableHealthPotion.additionalAttributes.Add(ItemAdditionalAttributes.OnUseConsume, 100);
        scriptableHealthPotion.additionalAttributes.Add(ItemAdditionalAttributes.OnUseHeal, 30);
        scriptableHealthPotion.Rarity = ItemRarity.Common;
        scriptableHealthPotion.itemName = "Health Potion";
        return scriptableHealthPotion;
    }

    private static ScriptableItem GenearateScriptableJewelry()
    {
        ScriptableItem scriptableJewelry = ScriptableObject.CreateInstance<ScriptableItem>();
        scriptableJewelry.Type = ItemType.Jewelry;
        scriptableJewelry.sprite = "Inventory/Sprites/" + scriptableJewelry.Type + "/" + Random.Range(0, 2);
        scriptableJewelry.Rarity = GenerateRarity();
        scriptableJewelry.itemName = scriptableJewelry.Rarity + " " + CreatePrefix() + scriptableJewelry.Type.ToString() + CreateSuffix();
        return scriptableJewelry;
    }

    private static ScriptableItem GenearateScriptableBomb()
    {
        ScriptableItem scriptableBomb = ScriptableObject.CreateInstance<ScriptableItem>();
        scriptableBomb.stackable = true;
        scriptableBomb.Type = ItemType.Bomb;
        scriptableBomb.sprite = "Inventory/Sprites/" + scriptableBomb.Type + "/" + 0;
        scriptableBomb.additionalAttributes.Add(ItemAdditionalAttributes.OnUseConsume, 100);
        scriptableBomb.additionalAttributes.Add(ItemAdditionalAttributes.OnUseDropBomb, 1);
        scriptableBomb.Rarity = ItemRarity.Common;
        scriptableBomb.itemName = "Bomb";
        return scriptableBomb;
    }

    private static ScriptableItem GenearateScriptableGold()
    {
        ScriptableItem scriptableGold = ScriptableObject.CreateInstance<ScriptableItem>();
        scriptableGold.Type = ItemType.Gold;
        scriptableGold.amount = Random.Range(1, 25);
        scriptableGold.stackable = true;
        scriptableGold.itemName = scriptableGold.Type.ToString();
        scriptableGold.sprite = "Inventory/Sprites/" + scriptableGold.Type + "/0";
        scriptableGold.Rarity = ItemRarity.Common;
        scriptableGold.itemName = "Gold";
        return scriptableGold;
    }

    public static ScriptableItem GenerateScriptableItem(ItemType itemType)
    {
        ScriptableItem scriptableItem;
        switch (itemType)
        {
            case ItemType.Weapon:
                scriptableItem = GenearateScriptableWeapon();
                break;
            case ItemType.RangedWeapon:
                scriptableItem = GenearateScriptableRangedWeapon();
                break;
            case ItemType.Helmet:
                scriptableItem = GenearateScriptableHelmet();
                break;
            case ItemType.Shield:
                scriptableItem = GenearateScriptableShield();
                break;
            case ItemType.Gold:
                scriptableItem = GenearateScriptableGold();
                break;
            case ItemType.Normal:
                scriptableItem = GenearateScriptableNormal();
                break;
            case ItemType.HealthPotion:
                scriptableItem = GenearateScriptableHealthPotion();
                break;
            case ItemType.Bomb:
                scriptableItem = GenearateScriptableBomb();
                break;
            case ItemType.Jewelry:
                scriptableItem = GenearateScriptableJewelry();
                break;
            case ItemType.Artifact:
                scriptableItem = GenearateScriptableArtifact();
                break;
            default:
                scriptableItem = ScriptableObject.CreateInstance<ScriptableItem>();
                break;
        }
        return scriptableItem;
    }

    public static ScriptableItem GenerateScriptableItem()
    {
        return GenerateScriptableItem((ItemType)Random.Range(0, numberOfItemTypes - 1));
    }

    public static Item GenerateItem()
    {
        return new(GenerateScriptableItem());
    }

    public static Item GenerateItem(ItemType itemType)
    {
        return new(GenerateScriptableItem(itemType));
    }
}
