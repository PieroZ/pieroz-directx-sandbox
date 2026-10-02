using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class ArmorDamageReductionAbility : MonoBehaviour
{
    public ConstrainedInventory[] armorInventories;
    [SerializeField] private int totalArmor = 0;

    private void OnArmorChanged()
    {
        int recalculatedTotalArmor = 0;
        foreach (ConstrainedInventory armorInventory in armorInventories)
        {
            Item armor = armorInventory.Get();
            if (armor != null)
            {

                if (armor.scriptableItem.additionalAttributes.ContainsKey(ItemAdditionalAttributes.Armor))
                {
                    recalculatedTotalArmor += armor.scriptableItem.additionalAttributes[ItemAdditionalAttributes.Armor];
                }
            }
        }
        totalArmor = recalculatedTotalArmor;
    }

    public int GetDamageReduction()
    {
        return totalArmor;
    }

    void Start()
    {
        foreach (ConstrainedInventory armorInventory in armorInventories)
        {
            armorInventory.OnInventoryChanged += OnArmorChanged;
        }
    }

    private void OnDestroy()
    {
        foreach (ConstrainedInventory armorInventory in armorInventories)
        {
            armorInventory.OnInventoryChanged -= OnArmorChanged;
        }
    }

}
