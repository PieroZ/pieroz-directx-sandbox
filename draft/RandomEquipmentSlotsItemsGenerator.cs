using System.Collections.Generic;
using UnityEngine;

public class RandomEquipmentSlotsItemsGenerator : MonoBehaviour
{
    public List<ConstrainedInventory> constrainedInventories = new();
    public float generateChance = 0.3f;

    private void Start()
    {
        foreach (ConstrainedInventory constrainedInventory in constrainedInventories)
        {
            if (Random.Range(0f, 1f) < generateChance && constrainedInventory != null)
            {
                constrainedInventory.Add(RandomItemGenerator.GenerateItem(constrainedInventory.GetrConstraints()[Random.Range(0, constrainedInventory.GetrConstraints().Count)]));
            }
        }
    }
}
