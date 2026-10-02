using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.EventSystems;

public class CanUseActionItemSlot : MonoBehaviour
{
    public ConstrainedInventory actionInventory;
    public KeyCode keyCode;

    private IEnumerator RepeatAfterCooldown(float time)
    {
        yield return new WaitForSeconds(time);
        if (Input.GetKey(keyCode))
        {
            if (!((keyCode == KeyCode.Mouse0 || keyCode == KeyCode.Mouse1) && EventSystem.current.IsPointerOverGameObject()))
            {
                TryUseItem(actionInventory.Get());
            }
        }
    }

    private void TryUseItem(Item actionItem)
    {
        if (actionItem != null)
        {
            if (actionItem.scriptableItem.additionalAttributes.ContainsKey(ItemAdditionalAttributes.OnUseCooldown))
            {
                var inventorySlotsUI = actionItem.GetInventorySlotUIs();
                if (inventorySlotsUI.Count > 0)
                {
                    foreach (InventorySlotUI inventorySlotUI in inventorySlotsUI)
                    {
                        if (inventorySlotUI.onCooldown)
                        {
                            return;
                        }
                    }
                    foreach (InventorySlotUI inventorySlotUI in inventorySlotsUI)
                    {
                        if (inventorySlotUI == null)
                        {
                            continue;
                        }
                        float cooldownTime = actionItem.scriptableItem.additionalAttributes[ItemAdditionalAttributes.OnUseCooldown];
                        inventorySlotUI.StartCooldown(cooldownTime);
                        StartCoroutine(RepeatAfterCooldown(cooldownTime / 5));
                    }
                    actionItem.Use(gameObject);
                }
            }
            else
            {
                actionItem.Use(gameObject);
            }
        }
    }

    private void Update()
    {
        if (Input.GetKeyDown(keyCode))
        {
            if ((keyCode == KeyCode.Mouse0 || keyCode == KeyCode.Mouse1) && EventSystem.current.IsPointerOverGameObject())
            {
                return;
            }
            TryUseItem(actionInventory.Get());
        }
    }
}
