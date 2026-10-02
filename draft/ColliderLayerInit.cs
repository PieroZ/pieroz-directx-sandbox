using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class ColliderLayerInit : MonoBehaviour
{
    public int offset = 0;
    private void Start()
    {
        gameObject.layer = transform.parent.gameObject.layer + offset;
        enabled = false;
    }
}
