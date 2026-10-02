using System.Collections.Generic;
using UnityEngine;
using UnityEngine.EventSystems;

public class InventoryUI : MonoBehaviour, IPointerDownHandler
{
    public InventorySlotUI inventorySlotPrefab;
    public GameObject inventoryHolder = null;
    public int widthOffset = 0;
    public int heightOffset = 0;
    private List<InventorySlotUI> slots = new();
    private Inventory inventory;
    private float inventorySlotWidth;
    private float inventorySlotHeight;
    public string InventoryTag = "general";
    public List<InventoryUI> relatedInventoryUIs = new();

    public bool HasConnectedInventory()
    {
        return inventory != null;
    }

    public bool IsEmpty()
    {
        if (HasConnectedInventory())
        {
            return inventory.IsEmpty();
        }
        return false;
    }

    public bool IsFull()
    {
        if (HasConnectedInventory())
        {
            return inventory.IsFull();
        }
        return true;
    }

    public Inventory GetInventory()
    {
        return inventory;
    }

    public void SetInventoryHolder(GameObject inventoryHolder)
    {
        if (!ReferenceEquals(this.inventoryHolder, inventoryHolder) && inventoryHolder.TryGetComponent(out Inventory inventory))
        {
            this.inventoryHolder = inventoryHolder;
            this.inventory = inventory;
            this.inventory.OnInventoryChanged += OnInventoryChanged;
            OnInventoryChanged();
        }
    }

    public bool IsSetInventoryHolder(GameObject inventoryHolder)
    {
        return ReferenceEquals(this.inventoryHolder, inventoryHolder);
    }

    public void UnsetInventoryHolder(GameObject inventoryHolder)
    {
        if (!ReferenceEquals(this.inventoryHolder, inventoryHolder))
        {
            return;
        }
        inventory.OnInventoryChanged -= OnInventoryChanged;
        inventory = null;
        this.inventoryHolder = null;
    }

    public void UnsetInventoryHolder()
    {
        inventory.OnInventoryChanged -= OnInventoryChanged;
        inventory = null;
        inventoryHolder = null;
    }

    void Start()
    {
        if (inventoryHolder == null)
        {
            inventoryHolder = PlayerManager.Instance.GetPlayerInventoryWithTag(InventoryTag).gameObject;
        }
        inventorySlotWidth = inventorySlotPrefab.GetComponent<RectTransform>().rect.width;
        inventorySlotHeight = inventorySlotPrefab.GetComponent<RectTransform>().rect.height;
        if (inventoryHolder && inventoryHolder.TryGetComponent(out inventory))
        {
            inventory.OnInventoryChanged += OnInventoryChanged;
            OnInventoryChanged();
        }
    }
    public void Remove(InventorySlotUI inventorySlotUI)
    {
        inventory.Remove(inventorySlotUI.item);
    }
    public void Add(Item item)
    {
        inventory.Add(item);
    }

    public void Add(InventorySlotUI inventorySlotUI)
    {
        inventory.Add(inventorySlotUI.item);
    }

    public void Drop(InventorySlotUI inventorySlotUI)
    {
        inventory.Drop(inventorySlotUI.item);
    }

    public InventoryUI GetRelatedInventoryUI(InventorySlotUI inventorySlotUI)
    {
        InventoryUI firstFound = null;
        InventoryUI firstEmpty = null;
        foreach (InventoryUI relatedInventoryUI in relatedInventoryUIs)
        {
            if (relatedInventoryUI.inventory != null && relatedInventoryUI.inventory is ConstrainedInventory && (relatedInventoryUI.inventory as ConstrainedInventory).GetrConstraints().Contains(inventorySlotUI.item.GetItemType()))
            {
                if (relatedInventoryUI.inventory.IsEmpty())
                {
                    if (firstEmpty == null)
                    {
                        firstEmpty = relatedInventoryUI;
                    }
                }
                else if (relatedInventoryUI.inventory is ConstrainedInventory && (relatedInventoryUI.inventory as ConstrainedInventory).Get().IsStackable() && (relatedInventoryUI.inventory as ConstrainedInventory).Get().GetItemType() == inventorySlotUI.item.GetItemType() && inventorySlotUI.item.IsStackable())
                {
                    return relatedInventoryUI;
                }
                else if (firstFound == null)
                {
                    firstFound = relatedInventoryUI;
                }
            }
        }
        if (firstEmpty != null)
        {
            return firstEmpty;
        }
        return firstFound;
    }

    public void Use(InventorySlotUI inventorySlotUI, GameObject target = null)
    {
        inventory.Use(inventorySlotUI.item, target);
    }

    public void OnInventoryChanged()
    {
        foreach (InventorySlotUI slotUI in slots)
        {
            slotUI.Destroy();
        }
        slots.Clear();
        slots = new();
        int i = 0;

        foreach (Item item in inventory.items)
        {
            Vector3 offset = new(Screen.height * (i % 8 * 1.1f * inventorySlotWidth + widthOffset) / 1080f, Screen.height * (i / 8 * -1.1f * inventorySlotHeight - heightOffset) / 1080f, 0);
            InventorySlotUI inventorySlotUI = Instantiate(inventorySlotPrefab, transform.position + offset, Quaternion.identity, transform);
            inventorySlotUI.Init(item);
            slots.Add(inventorySlotUI);
            i++;
        }
    }

    private void OnDestroy()
    {
        if (inventory)
        {
            inventory.OnInventoryChanged -= OnInventoryChanged;
        }
    }

    public int GetIndexOf(Item item)
    {
        return inventory.GetIndexOf(item);
    }

    public void SwapItemWithFloatingItem(Item item)
    {
        Item floatingItem = FloatingItemPickerManagerUI.Instance.GetFloatingItem();
        if (floatingItem.IsStackable() && floatingItem.GetItemType() == item.GetItemType())
        {
            AddFromFloatingItem(inventory.GetIndexOf(item));
        }
        else
        {
            inventory.SetCapacity(inventory.GetCapacity() + 1);
            if (AddFromFloatingItem(inventory.GetIndexOf(item)))
            {
                FloatingItemPickerManagerUI.Instance.AddFloatingIcon(item);
            }
            inventory.SetCapacity(inventory.GetCapacity() - 1);
        }
    }

    private bool AddFromFloatingItem(int index = -1)
    {
        Item floatableItem = FloatingItemPickerManagerUI.Instance.GetFloatingItem();
        if (floatableItem == null)
        {
            return false;
        }
        Inventory itemInventory = floatableItem.GetInventory();
        if (inventory.Add(floatableItem, index))
        {
            itemInventory.Remove(floatableItem);
            AudioManager.Instance.Play("ItemDrop");
            return true;
        }
        return false;
    }

    public void OnPointerDown(PointerEventData eventData)
    {
        AddFromFloatingItem();
    }
}
