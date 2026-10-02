using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class LevelMapManager : MonoBehaviour
{
    static bool isActive = true;
    public LevelMap levelMap;
    void Start()
    {
        if (WorldMapManager.Instance != null)
        {
            levelMap.gameObject.SetActive(isActive);
        }
        else
        {
            levelMap.gameObject.SetActive(false);
        }
    }

    void Update()
    {
        if (Input.GetKeyDown(KeyCode.M) && WorldMapManager.Instance != null)
        {
            isActive = !isActive;
            levelMap.gameObject.SetActive(isActive);
        }
    }
}
