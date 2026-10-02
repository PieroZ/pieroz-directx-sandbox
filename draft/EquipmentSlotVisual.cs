using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Rendering.Universal;

public class EquipmentSlotVisual : MonoBehaviour
{
    public ConstrainedInventory equipmentSlot = null;
    
    private void OnEquipmentChanged()
    {
        Item item = equipmentSlot.Get();
        if (item == null)
        {
            GetComponent<SpriteRenderer>().sprite = null;
            if (TryGetComponent(out ShadowCaster2D shadowCaster2D))
            {
                shadowCaster2D.enabled = false;
            }
        }
        else
        {
            GetComponent<SpriteRenderer>().sprite = item.GetSprite();
            if (TryGetComponent(out ShadowCaster2D shadowCaster2D))
            {
                shadowCaster2D.enabled = true;
            }
        }

    }

    void Start()
    {
        if (equipmentSlot != null)
        {
            equipmentSlot.OnInventoryChanged += OnEquipmentChanged;
            OnEquipmentChanged();
        }
    }

    void OnDestroy()
    {
        if (equipmentSlot != null)
        {
            equipmentSlot.OnInventoryChanged -= OnEquipmentChanged;
        }
    }
}
