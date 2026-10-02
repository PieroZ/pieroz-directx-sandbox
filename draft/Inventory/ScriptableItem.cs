using System;
using System.Collections.Generic;
using UnityEngine;

[Serializable, CreateAssetMenu(fileName = "ScriptableItem", menuName = "Scriptable Objects/Scriptable Item")]
public class ScriptableItem : ScriptableObject
{
    public ItemType Type = ItemType.Normal;
    public ItemRarity Rarity = ItemRarity.NumberOfRarityClasses;
    public string itemName = "DefaultName";
    public string sprite = "Inventory/Sprites/gem";
    public bool stackable = false;
    public int amount = 1;
    public int limit = 1;
    public float weight = 1f;
    public float value = 1f;
    public Dictionary<ItemAdditionalAttributes, int> additionalAttributes = new();

    public virtual SerializableScriptableItem Serialize()
    {
        SerializableScriptableItem scriptableItem = new()
        {
            Type = Type,
            Rarity = Rarity,
            itemName = itemName,
            sprite = sprite,
            stackable = stackable,
            amount = amount,
            limit = limit,
            weight = weight,
            value = value,
            additionalAttributes = additionalAttributes
        };

        return scriptableItem;
    }
}
