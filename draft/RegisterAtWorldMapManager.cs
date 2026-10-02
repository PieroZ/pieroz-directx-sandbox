using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class RegisterAtWorldMapManager : MonoBehaviour
{
    private void Start()
    {
        if (transform.parent.name != "WorldMap" && !WorldMapManager.Instance.TryRegisterPermamentInstantiatedGameObject(gameObject))
        {
            Destroy(gameObject);
        }
        enabled = false;
    }
}
