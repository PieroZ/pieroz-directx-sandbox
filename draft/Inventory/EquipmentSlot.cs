using System.Collections;
using System.Collections.Generic;
using UnityEngine;

[System.Serializable]
public class EquipmentSlot
{
    public ItemType slotType;
    public Item item = null;

    public bool Equip(Item item)
    {
        if (item.GetItemType() == slotType)
        {
            this.item = item;
            Debug.Log("Equiped " + item.GetItemName() + " " + slotType);
            return true;
        }
        return false;
    }

    public Item Unequip()
    {
        Item item = this.item;
        this.item = null;
        Debug.Log("Unequiped " + item.GetItemName());
        return item;
    }

    public bool IsEquiped()
    {
        return item != null;
    }

    public bool IsEquiped(Item item)
    {
        return ReferenceEquals(this.item, item);
    }
}
