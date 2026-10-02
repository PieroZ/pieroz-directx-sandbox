using UnityEngine;
public class ChestInteractableController : MonoBehaviour, IInteractable
{
    public bool IsInUse()
    {
        return InventoryManagerUI.Instance.IsSetSecondaryInventoryHolder(gameObject);
    }

    public void Use()
    {
        InventoryManagerUI.Instance.SetSecondaryInventoryHolder(gameObject);
    }

    public void Unuse()
    {
        InventoryManagerUI.Instance.UnsetSecondaryInventoryHolder(gameObject);
    }
}
