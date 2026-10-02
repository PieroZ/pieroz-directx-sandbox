using System.Collections.Generic;
using System;
using UnityEngine;

[Serializable]
public class Item
{
    public ScriptableItem scriptableItem;
    private Inventory inventory = null;
    private List<InventorySlotUI> inventorySlotUIs = new();

    public List<InventorySlotUI> GetInventorySlotUIs()
    {
        return inventorySlotUIs;
    }
    public void AddInventorySlotUI(InventorySlotUI inventorySlotUI)
    {
        if (inventorySlotUIs == null)
        {
            inventorySlotUIs = new();
        }
        if (!inventorySlotUIs.Contains(inventorySlotUI))
        {
            inventorySlotUIs.Add(inventorySlotUI);
        }
    }

    public void RemoveInventorySlotUI(InventorySlotUI inventorySlotUI)
    {
        if (inventorySlotUIs == null)
        {
            return;
        }
        if (inventorySlotUIs.Contains(inventorySlotUI))
        {
            inventorySlotUIs.Remove(inventorySlotUI);
        }
    }

    public Item(ScriptableItem scriptableItem)
    {
        this.scriptableItem = scriptableItem;
    }

    public void SetInventory(Inventory inventory)
    {
        this.inventory = inventory;
    }

    public Inventory GetInventory()
    {
        return inventory;
    }

    public bool HasAdditionalParameter(ItemAdditionalAttributes itemAdditionalAttribute)
    {
        return scriptableItem.additionalAttributes.ContainsKey(itemAdditionalAttribute);
    }

    public bool GetAdditionalParameter(ItemAdditionalAttributes itemAdditionalAttribute, out int additionalAtributeValue)
    {
        return scriptableItem.additionalAttributes.TryGetValue(itemAdditionalAttribute, out additionalAtributeValue);
    }

    public int GetAdditionalParameter(ItemAdditionalAttributes itemAdditionalAttribute)
    {
        return scriptableItem.additionalAttributes.GetValueOrDefault(itemAdditionalAttribute);
    }
    private void AddToItemActionEffect(ref ItemActionEffect itemActionEffect, GameObject target, ItemAdditionalAttributes attribute, int value = 0)
    {
        if (itemActionEffect == null)
        {
            itemActionEffect = UnityEngine.Object.Instantiate(Resources.Load<ItemActionEffect>("Prefabs/ItemActionEffect"), target.transform.position, Quaternion.identity);
        }
        itemActionEffect.AddToItemActionEffect(target, attribute, value);
    }

    public void Use(GameObject target = null)
    {
        bool removeAfterUse = false;
        bool initAfterCreation = true;
        ItemActionEffect itemActionEffect = null;
        foreach (KeyValuePair<ItemAdditionalAttributes, int> attribute in scriptableItem.additionalAttributes)
        {
            switch (attribute.Key)
            {
                case ItemAdditionalAttributes.OnUseConsume:
                    {
                        removeAfterUse = true;
                    }
                break;
                case ItemAdditionalAttributes.OnUseHeal:
                    if (target != null && target.TryGetComponent(out CharacterHealth characterHealth))
                    {
                        AudioManager.Instance.Play("PotionDrink");
                        characterHealth.Heal(attribute.Value);
                    }
                break;
                case ItemAdditionalAttributes.OnUseDropBomb:
                    if (target != null)
                    {
                        UnityEngine.Object.Instantiate(Resources.Load<Explodeable>("Prefabs/Bomb"), target.transform.position, Quaternion.identity);
                    }
                break;
                case ItemAdditionalAttributes.UseShotAtTarget:
                    initAfterCreation = false;
                    AddToItemActionEffect(ref itemActionEffect, target, attribute.Key);
                    break;
                default:
                    AddToItemActionEffect(ref itemActionEffect, target, attribute.Key, attribute.Value);
                    break;
            }
        }

        if (itemActionEffect != null && initAfterCreation)
        {
            itemActionEffect.Init();
        }

        if (removeAfterUse)
        {
            if (IsStackable())
            {
                RemoveAmount(1);
                if (GetAmount() <= 0)
                {
                    inventory.Remove(this);
                }
                else
                {
                    inventory.ItemChanged(this);
                }
            }
            else
            {
                inventory.Remove(this);
            }
        }
    }

    public ItemPicker CreateItemPicker(Vector3 position)
    {
        ItemPicker itemPicker = UnityEngine.Object.Instantiate(Resources.Load<ItemPicker>("Inventory/_ItemPicker"), position, Quaternion.identity);
        itemPicker.scriptableItem = scriptableItem;
        itemPicker.GetComponent<SpriteRenderer>().sprite = GetSprite();
        itemPicker.Init();

        return itemPicker;
    }

    public Sprite GetSprite()
    {
        try
        {
            return UnityEngine.Object.Instantiate(Resources.Load<Sprite>(scriptableItem.sprite));
        }
        catch
        {
            Debug.Log(scriptableItem.sprite);
            Debug.Log(scriptableItem.itemName + " " + scriptableItem.Type.ToString());
            return null;
        }
    }

    public ItemType GetItemType()
    {
        return scriptableItem.Type;
    }

    public string GetItemName()
    {
        return scriptableItem.itemName;
    }

    public bool IsStackable()
    {
        return scriptableItem.stackable;
    }

    public int GetAmount()
    {
        return scriptableItem.amount;
    }

    public void RemoveAmount(int amount)
    {
        scriptableItem.amount -= amount;
    }

    public void AddAmout(int amount)
    {
        scriptableItem.amount += amount;
    }

    public void SetAmout(int amount)
    {
        scriptableItem.amount = amount;
    }

    public int GetLimit()
    {
        return scriptableItem.limit;
    }

    public ItemRarity GetRarity()
    {
        return scriptableItem.Rarity;
    }
}
