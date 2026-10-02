using UnityEngine;

public static class ItemRarityToColor
{   public static Color ToColor(ItemRarity rarity)
    {
        switch (rarity)
        {
            case ItemRarity.Legendary:
                return Color.red;
            case ItemRarity.Epic:
                return Color.yellow;
            case ItemRarity.Rare:
                return Color.green;
            case ItemRarity.Uncommon:
                return Color.blue;
            default:
                return Color.white;
        }
    }
}
