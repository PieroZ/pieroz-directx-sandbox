using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class Inventory : MonoBehaviour
{
    public int capacity = 16;
    [SerializeField] public List<Item> items = new();
    public delegate void OnInventoryChangedHandler();
    public OnInventoryChangedHandler OnInventoryChanged;
    public string inventoryTag = "general";
    public List<Inventory> relatedInventories = new();

    private void UpdateItemsOwnerships()
    {
        foreach (Item item in items)
        {
            item.SetInventory(this);
        }
    }

    private void Awake()
    {
        UpdateItemsOwnerships();
    }

    public bool IsFull()
    {
        return GetCount() == GetCapacity();
    }

    public bool IsEmpty()
    {
        return GetCount() == 0;
    }

    public int GetCount()
    {
        return items.Count;
    }

    public int GetCapacity()
    {
        return capacity;
    }

    public int GetIndexOf(Item item)
    {
        return items.IndexOf(item);
    }

    public void SetCapacity(int capacity)
    {
        this.capacity = capacity;
    }

    private bool AddStackable(Item item)
    {
        if (item.IsStackable())
        {
            Item foundItem = items.Find(x => x.GetItemType() == item.GetItemType());
            if (foundItem != null)
            {
                foundItem.AddAmout(item.GetAmount());
                return true;
            }
        }
        return false;
    }

    public bool Contains(Item item)
    {
        return items.Contains(item);
    }

    private Inventory GetRelatedInventory(Item item)
    {
        Inventory firstEmpty = null;
        foreach (Inventory relatedInventory in relatedInventories)
        {
            if (relatedInventory != null && relatedInventory is ConstrainedInventory && (relatedInventory as ConstrainedInventory).GetrConstraints().Contains(item.GetItemType()))
            {
                if (relatedInventory.IsEmpty())
                {
                    if (firstEmpty == null)
                    {
                        firstEmpty = relatedInventory;
                    }
                }
                else if (relatedInventory is ConstrainedInventory && (relatedInventory as ConstrainedInventory).Get().IsStackable() && (relatedInventory as ConstrainedInventory).Get().GetItemType() == item.GetItemType() && item.IsStackable())
                {
                    return relatedInventory;
                }
            }
        }
        return firstEmpty;
    }

    public virtual bool Add(Item item, int index = -1)
    {
        if (index < 0 && FloatingItemPickerManagerUI.Instance != null && !FloatingItemPickerManagerUI.Instance.IsFloating() && (InventoryManagerUI.Instance == null || InventoryManagerUI.Instance.secondaryInventoryUI == null || InventoryManagerUI.Instance.secondaryInventoryUI.inventoryHolder == null))
        {
            Inventory relatedInventory = GetRelatedInventory(item);
            if (relatedInventory != null)
            {
                return relatedInventory.Add(item);
            }
        }
        return AddAvoidRelativeInventory(item, index);
    }

    public bool AddAvoidRelativeInventory(Item item, int index = -1)
    {
        if (!AddStackable(item))
        {
            if (items.Count >= capacity)
            {
                return false;
            }
            if (index < 0)
            {
                items.Add(item);
            }
            else
            {
                items.Insert(index, item);
            }
        }
        item.SetInventory(this);
        OnInventoryChanged?.Invoke();
        return true;
    }

    public void Use(Item item, GameObject target = null)
    {
        item.Use(target);
    }

    public bool Drop(Item item) 
    {
        if (!Remove(item))
        {
            return false;
        }
        float angle = Random.Range(0f, 2f * Mathf.PI);
        float radius = 0.16f;
        item.CreateItemPicker(transform.position + radius * new Vector3(Mathf.Cos(angle), Mathf.Sin(angle), 0));

        AudioManager.Instance.Play("ItemDrop");
        return true;
    }

    public void Drop()
    {
        while (items.Count > 0)
        {
            Drop(items[0]);
        }
    }

    public void ItemChanged(Item item)
    {
        OnInventoryChanged?.Invoke();
    }

    public bool Remove(Item item)
    {
        if (item == null || !items.Remove(item))
        {
            return false;
        }
        OnInventoryChanged?.Invoke();
        return true;
    }

    public void RemoveAll()
    {
        while (items.Count > 0)
        {
            Remove(items[0]);
        }
    }
}
