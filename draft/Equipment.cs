using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class Equipment
{
    public List<EquipmentSlot> equipmentSlots = null;

    public bool IsEquiped(Item item)
    {
        if (equipmentSlots == null)
        {
            return false;
        }
        foreach (EquipmentSlot equipmentSlot in equipmentSlots)
        {
            if (equipmentSlot.IsEquiped(item))
            {
                return true;
            }
        }
        return false;
    }

    public bool Unequip(Item item)
    {
        if (equipmentSlots == null)
        {
            return false;
        }
        foreach (EquipmentSlot equipmentSlot in equipmentSlots)
        {
            if (equipmentSlot.IsEquiped(item))
            {
                return equipmentSlot.Unequip() != null;
            }
        }
        return false;
    }

    public bool Equip(Item item)
    {
        if (equipmentSlots == null)
        {
            return false;
        }
        foreach (EquipmentSlot equipmentSlot in equipmentSlots)
        {
            if (!equipmentSlot.IsEquiped(item) && equipmentSlot.Equip(item))
            {
                return true;
            }
        }
        return false;
    }
}
