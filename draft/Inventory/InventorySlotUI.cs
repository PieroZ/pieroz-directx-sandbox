using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.UI;
using TMPro;
using System.Collections;

public class InventorySlotUI : MonoBehaviour, IPointerDownHandler, IPointerEnterHandler, IPointerExitHandler
{
    public Image image;
    public Image frame;
    public Item item = null;
    public TMP_Text amount;
    [SerializeField] public Animator cooldownAnimationPrefab;
    Animator cooldownAnimation;
    public bool onCooldown = false;
    public bool isInit = false;
    
    private void ClearCooldown()
    {
        onCooldown = false;
        if (cooldownAnimation != null)
        {
            Destroy(cooldownAnimation.gameObject);
        }
    }

    private IEnumerator Cooldown(float time)
    {
        yield return new WaitForSeconds(time);
        ClearCooldown();
    }

    public void StartCooldown(float time)
    {
        if (!gameObject.activeInHierarchy)
        {
            return;
        }
        time /= 5f;
        onCooldown = true;
        StartCoroutine(Cooldown(time));
        cooldownAnimation = Instantiate(cooldownAnimationPrefab, transform);
        cooldownAnimation.speed = 1f / time;
    }

    public void OnPointerDown(PointerEventData eventData)
    {
        if (eventData.button == PointerEventData.InputButton.Left)
        {
            HandleLeftClick();
        }
        else if (eventData.button == PointerEventData.InputButton.Right)
        {
            if (Input.GetKey(KeyCode.LeftControl))
            {
                HandleRightClickWithLeftControl();
            }
            else if (Input.GetKey(KeyCode.LeftShift) && InventoryManagerUI.Instance.inventoryUI != null && InventoryManagerUI.Instance.inventoryUI.HasConnectedInventory())
            {
                HandleRightClickWithLeftShift();
            }
            else
            {
                HandleRightClick();
            }
        }
        AudioManager.Instance.Play("ItemDrop");
    }

    private void HandleRightClick()
    {
        if (item.scriptableItem.additionalAttributes.ContainsKey(ItemAdditionalAttributes.OnUseCooldown))
        {
            if (onCooldown)
            {
                return;
            }
            StartCooldown(item.scriptableItem.additionalAttributes[ItemAdditionalAttributes.OnUseCooldown]);
        }
        GetComponentInParent<InventoryUI>().Use(this, PlayerManager.Instance.GetPlayerController().gameObject);
    }

    private void HandleLeftClick()
    {
        if (FloatingItemPickerManagerUI.Instance.IsFloating())
        {
            GetComponentInParent<InventoryUI>().SwapItemWithFloatingItem(item);
        }
        else
        {
            FloatingItemPickerManagerUI.Instance.AddFloatingIcon(item);
        }
    }

    private void HandleRightClickWithLeftShift()
    {
        InventoryUI relatedInventoryUI;

        if (ReferenceEquals(InventoryManagerUI.Instance.secondaryInventoryUI, GetComponentInParent<InventoryUI>()))
        {
            relatedInventoryUI = InventoryManagerUI.Instance.inventoryUI.GetRelatedInventoryUI(this);
        }
        else
        {
            relatedInventoryUI = GetComponentInParent<InventoryUI>().GetRelatedInventoryUI(this);
        }
        Item itemToReplace = null;
        if (relatedInventoryUI != null)
        {
            if (!relatedInventoryUI.IsEmpty())
            {
                Inventory relatedInventory = relatedInventoryUI.GetInventory();
                if (relatedInventory is ConstrainedInventory)
                {
                    itemToReplace = (relatedInventory as ConstrainedInventory).Get();
                    if (itemToReplace.IsStackable() && itemToReplace.GetItemType() == this.item.GetItemType() && this.item.IsStackable())
                    {
                        itemToReplace = null;
                    }
                    else
                    {
                        (relatedInventory as ConstrainedInventory).Remove();
                    }
                }
            }
            GetComponentInParent<InventoryUI>().Remove(this);
            relatedInventoryUI.Add(this);
            if (itemToReplace != null)
            {
                GetComponentInParent<InventoryUI>().Add(itemToReplace);
            }
        }
        else
        {
            if (!InventoryManagerUI.Instance.inventoryUI.IsFull())
            {
                GetComponentInParent<InventoryUI>().Remove(this);
                InventoryManagerUI.Instance.inventoryUI.Add(this);
            }
        }
    }

    private void HandleRightClickWithLeftControl()
    {
        if (!ReferenceEquals(InventoryManagerUI.Instance.secondaryInventoryUI, GetComponentInParent<InventoryUI>()) && InventoryManagerUI.Instance.secondaryInventoryUI != null && InventoryManagerUI.Instance.secondaryInventoryUI.HasConnectedInventory())
        {
            if (InventoryManagerUI.Instance.secondaryInventoryUI.GetInventory().GetCount() < InventoryManagerUI.Instance.secondaryInventoryUI.GetInventory().GetCapacity())
            {
                GetComponentInParent<InventoryUI>().Remove(this);
                InventoryManagerUI.Instance.secondaryInventoryUI.Add(this);
            }
        }
        else if (ReferenceEquals(InventoryManagerUI.Instance.secondaryInventoryUI, GetComponentInParent<InventoryUI>()) && InventoryManagerUI.Instance.inventoryUI != null && InventoryManagerUI.Instance.inventoryUI.HasConnectedInventory())
        {
            if (InventoryManagerUI.Instance.inventoryUI.GetInventory().GetCount() < InventoryManagerUI.Instance.inventoryUI.GetInventory().GetCapacity())
            {
                GetComponentInParent<InventoryUI>().Remove(this);
                InventoryManagerUI.Instance.inventoryUI.Add(this);
            }
        }
        else
        {
            GetComponentInParent<InventoryUI>().Drop(this);
        }
    }

    private void OnEnable()
    {
        if (isInit && item != null)
        {
            item.AddInventorySlotUI(this);
        }
    }
 
    public void Init(Item it)
    {
        item = it;
        item.AddInventorySlotUI(this);
        image.sprite = item.GetSprite();
        if (item.GetAmount() > 1)
        {
            amount.text = item.GetAmount().ToString();
        }
        else
        {
            amount.text = "";
        }
        frame.color = ItemRarityToColor.ToColor(item.GetRarity());
        isInit = true;
    }

    public void Destroy()
    {
        Destroy(gameObject);
    }

    public void OnPointerEnter(PointerEventData eventData)
    {
        ItemTooltipManagerUI.Instance.ShowTooltip(this);
    }

    public void OnPointerExit(PointerEventData eventData)
    {
        ItemTooltipManagerUI.Instance.HideTooltip();
    }

    private void OnDestroy()
    {
        ItemTooltipManagerUI.Instance.HideTooltip();
        ClearCooldown();
        if (item != null)
        {
            item.RemoveInventorySlotUI(this);
        }
    }

    private void OnDisable()
    {
        ItemTooltipManagerUI.Instance.HideTooltip();
        ClearCooldown();
        if (item != null)
        {
            item.RemoveInventorySlotUI(this);
        }
    }
}
