using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class ConstrainedInventory : Inventory
{
    [SerializeField] private List<ItemType> constraints = new();

    public override bool Add(Item item, int index = -1)
    {
        if (constraints.Count > 0 && !constraints.Contains(item.GetItemType()))
        {
            return false;
        }
        return base.Add(item, index);
    }

    public List<ItemType> GetrConstraints()
    {
        return constraints;
    }

    public Item Get()
    {
        if (GetCount() > 0)
        {
            return items[0];
        }
        return null;
    }

    public bool Remove()
    {
        return Remove(Get());
    }
}
