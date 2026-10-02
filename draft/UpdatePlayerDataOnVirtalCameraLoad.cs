using Unity.Cinemachine;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class UpdatePlayerDataOnVirtalCameraLoad : MonoBehaviour
{
    public static UpdatePlayerDataOnVirtalCameraLoad Instance { get; private set; }
    void Awake()
    {
        if (Instance == null)
        {
            Instance = this;
        }
        else
        {
            Destroy(gameObject);
            return;
        }
    }

    public void OverridePlayerData(Transform t)
    {
        GetComponent<CinemachineVirtualCamera>().Follow = t;
    }

    public void UpdatePlayerData()
    {
        if (PlayerManager.Instance != null)
        {
            GetComponent<CinemachineVirtualCamera>().Follow = PlayerManager.Instance.GetPlayerController().transform;
        }
    }

    void Start()
    {
        if (GetComponent<CinemachineVirtualCamera>().Follow == null)
        {
            UpdatePlayerData();
        }
        enabled = false;
    }
}
