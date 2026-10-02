using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class CanSpawnObject : MonoBehaviour
{
    public GameObject spawnGameObjectPrefab;
    public KeyCode keyCode;

    private void Update()
    {
        if (Input.GetKeyDown(keyCode))
        {
            Instantiate(spawnGameObjectPrefab, transform.position, Quaternion.identity);
        }
    }
}
