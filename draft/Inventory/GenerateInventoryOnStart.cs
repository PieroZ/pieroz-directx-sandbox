using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class GenerateInventoryOnStart : MonoBehaviour
{

    private void Start()
    {
        if (TryGetComponent(out Inventory inventory))
        {
            while (!inventory.IsFull())
            {
                inventory.Add(RandomItemGenerator.GenerateItem());
            }
        }
        enabled = false;
    }
}

