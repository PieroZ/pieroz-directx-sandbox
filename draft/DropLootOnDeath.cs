using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class DropLootOnDeath : MonoBehaviour
{
    public ScriptableItem scriptableItemToDrop;
    private CharacterHealth characterHealth = null;
    public List<Inventory> inventories = new();
    void DropLoot()
    {
        Item item = RandomItemGenerator.GenerateItem();
        float angle = Random.Range(0f, 2f * Mathf.PI);
        float radius = 0.16f;
        item.CreateItemPicker(transform.position + radius * new Vector3(Mathf.Cos(angle), Mathf.Sin(angle), 0));
        foreach (Inventory inventory in inventories)
        {
            inventory.Drop();
        }    
    }

    void Start()
    {
        if (TryGetComponent(out characterHealth))
        {
            characterHealth.OnDied += DropLoot;
        }
    }

    private void OnDestroy()
    {
        if (characterHealth != null)
        {
            characterHealth.OnDied -= DropLoot;
        }    
    }
}
