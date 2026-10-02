using UnityEngine;
using UnityEngine.EventSystems;

public class ItemPicker : MonoBehaviour, IInteractable
{
    public ScriptableItem scriptableItem = null;
    public PhysicsMaterial2D physicsMaterial2D;
    public TooltipManagerUI itemPickupTooltipManagerUI;
    private Inventory collidingInventory = null;
    private Focusable focusable = null;
    private VisibleToPlayer visibleToPlayer = null;
    private bool isVisibleByPlayer = true;
    public bool forceShowTooltip = false;

    public void ForceMouseSensitivity()
    {
        itemPickupTooltipManagerUI.ForceMouseSensitivity(this);
    }

    public void UnforceMouseSensitivity()
    {
        itemPickupTooltipManagerUI.UnforceMouseSensitivity(this);
    }


    public Vector3 GetTooltipPosition()
    {
        return itemPickupTooltipManagerUI.GetPosition(this);
    }

    public void Select()
    {
        itemPickupTooltipManagerUI.Select(this);
    }

    public void Deselect()
    {
        itemPickupTooltipManagerUI.Deselect(this);
    }

    private bool IsAutoPickable()
    {
        return scriptableItem != null && (scriptableItem.Type == ItemType.Gold);// || scriptableItem.Type == ItemType.Bomb || scriptableItem.Type == ItemType.HealthPotion);
    }

    private void Awake()
    {
        if (scriptableItem == null)
        {
            scriptableItem = RandomItemGenerator.GenerateScriptableItem();
            GetComponent<SpriteRenderer>().sprite = Instantiate(Resources.Load<Sprite>(scriptableItem.sprite));
        }
    }

    private void Start()
    {
        ItemPickerManager.Instance.RegisterItemPicker(this);
        if (TryGetComponent(out focusable))
        {
            focusable.FocusLost += OnFocusLost;
        }
        if (TryGetComponent(out visibleToPlayer))
        {
            visibleToPlayer.OnSpottebByPlayer += OnSpottebByPlayer;
            visibleToPlayer.OnSightLostByPlayer += OnSightLostByPlayer;
        }
    }

    void OnSpottebByPlayer()
    {
        //isVisibleByPlayer = true;
        /*if (Input.GetKey(KeyCode.LeftAlt))
        {
            itemPickupTooltipManagerUI.ShowTooltip(this);
        }*/
    }

    void OnSightLostByPlayer()
    {
        //isVisibleByPlayer = false;
        //itemPickupTooltipManagerUI.HideTooltip(this);
    }

    private void TryAddItem()
    {
        Item item = new(scriptableItem);
        if (collidingInventory.Add(item))
        {
            AudioManager.Instance.Play("ItemPickup");
            Destroy(gameObject);
        }
    }

    public void TryAddItem(Inventory inventory)
    {
        Item item = new(scriptableItem);
        if (inventory.Add(item))
        {
            Destroy(gameObject);
        }
    }

    private void TryAddItemToPlayerInventory()
    {
        PlayerManager.Instance.GetPlayerController().TryGetComponent(out collidingInventory);
        if (Vector2.Distance(collidingInventory.transform.position, transform.position) > .64f)
        {
            return;
        }
        if (ControlModeManager.Instance.controlMode == ControlMode.NormalMode)
        {
            TryAddItem();
        }
        else if (ControlModeManager.Instance.controlMode == ControlMode.InventoryMode)
        {
            if (FloatingItemPickerManagerUI.Instance.AddFloatingIcon(new Item(scriptableItem)))
            {
                AudioManager.Instance.Play("ItemPickup");
                Destroy(gameObject);
            }
        }
    }

    private void TryAddItem(Collider2D collision, bool autoPickable = false)
    {
        if (!collision.isTrigger)
        {
            return;
        }
        if (ControlModeManager.Instance.controlMode == ControlMode.NormalMode && collision.TryGetComponent(out collidingInventory) && (focusable != null && focusable.IsFocused() || (autoPickable && IsAutoPickable())))
        {
            TryAddItem();
        }
        if (ControlModeManager.Instance.controlMode == ControlMode.InventoryMode && collision.TryGetComponent(out collidingInventory) && (focusable != null && focusable.IsFocused()))
        { 
            if (FloatingItemPickerManagerUI.Instance.AddFloatingIcon(new Item(scriptableItem)))
            {
                AudioManager.Instance.Play("ItemPickup");
                Destroy(gameObject);
            }
        }
    }

    void OnTriggerEnter2D(Collider2D collision)
    {
        TryAddItem(collision, true);
    }

    private void OnTriggerExit2D(Collider2D collision)
    {
        collidingInventory = null;
    }

    private void OnTriggerStay2D(Collider2D collision)
    {
        TryAddItem(collision, false);
    }

    public void ForceShowTooltip()
    {
        forceShowTooltip = true;
        ShowTooltip();
    }

    public void UnforceShowTooltip()
    {
        forceShowTooltip = false;
        if ((!Input.GetKey(KeyCode.LeftAlt) || !visibleToPlayer))
        {
            HideTooltip();
        }
    }

    public void ShowTooltip()
    {
        itemPickupTooltipManagerUI.ShowTooltip(this);
    }

    public void HideTooltip()
    {
        itemPickupTooltipManagerUI.HideTooltip(this);
    }

    private void Update()
    {
        if (isVisibleByPlayer && Input.GetKeyDown(KeyCode.LeftAlt))
        {
            ShowTooltip();
        }
        if ((Input.GetKeyUp(KeyCode.LeftAlt) || !visibleToPlayer) && !forceShowTooltip)
        {
            HideTooltip();
        }
    }

    public void Init()
    {
        if (isVisibleByPlayer && Input.GetKey(KeyCode.LeftAlt))
        {
            ShowTooltip();
        }
    }

    private void OnMouseEnter()
    {
        if (isVisibleByPlayer)
        {
            ShowTooltip();
        }
/*        if (!EventSystem.current.IsPointerOverGameObject())
        {
            itemPickupTooltipManagerUI.MouseOverTooltip(this);
        }*/
    }

    private void OnMouseExit()
    {
        if (!Input.GetKey(KeyCode.LeftAlt) && !forceShowTooltip)
        {
            HideTooltip();
        }
        //itemPickupTooltipManagerUI.MouseOutOffTooltip(this);
    }

    private void OnMouseDown()
    {
        if (!EventSystem.current.IsPointerOverGameObject())
        {
            TryAddItemToPlayerInventory();
        }
    }

    private void OnDestroy()
    {
        ItemPickerManager.Instance.UnregisterItemPicker(this);
        if (visibleToPlayer != null)
        {
            visibleToPlayer.OnSpottebByPlayer -= OnSpottebByPlayer;
            visibleToPlayer.OnSightLostByPlayer -= OnSightLostByPlayer;
        }
        if (focusable != null)
        {
            focusable.FocusLost -= OnFocusLost;
        }
        HideTooltip();

    }

    public void OnFocusLost()
    { 
        gameObject.GetComponent<SpriteRenderer>().enabled = true;
        if (isVisibleByPlayer && Input.GetKey(KeyCode.LeftAlt))
        {
            ShowTooltip();
        }
    }

    public bool IsInUse()
    {
        return false;
    }

    public void Use()
    {
        TryAddItemToPlayerInventory();
    }

    public void Unuse()
    {
    }
}
