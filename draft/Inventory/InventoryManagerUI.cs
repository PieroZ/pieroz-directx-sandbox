using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

public class InventoryManagerUI : MonoBehaviour
{
    public InventoryUI inventoryUI;
    public InventoryUI secondaryInventoryUI;
    public static InventoryManagerUI Instance { get; private set; }

    private void Awake()
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
        inventoryUI.gameObject.SetActive(false);
        if (inventoryUI.gameObject.activeInHierarchy)
        {
            ControlModeManager.Instance.SetControlMode(ControlMode.InventoryMode);
        }
        if (secondaryInventoryUI.inventoryHolder == null)
        {
            secondaryInventoryUI.gameObject.SetActive(false);
        }
    }

    public void SetSecondaryInventoryHolder(GameObject inventoryHolder)
    {
        secondaryInventoryUI.SetInventoryHolder(inventoryHolder);
        if (!secondaryInventoryUI.gameObject.activeInHierarchy)
        {
            secondaryInventoryUI.gameObject.SetActive(true);
            ControlModeManager.Instance.SetControlMode(ControlMode.InventoryMode);
            inventoryUI.gameObject.SetActive(true);
            AudioManager.Instance.Play("ChestOpen");

        }
    }
    public void UnsetSecondaryInventoryHolder(GameObject inventoryHolder)
    {
        secondaryInventoryUI.UnsetInventoryHolder(inventoryHolder);
        if (secondaryInventoryUI != null && secondaryInventoryUI.gameObject.activeInHierarchy)
        {
            secondaryInventoryUI.gameObject.SetActive(false);
            ControlModeManager.Instance.SetControlMode(ControlMode.NormalMode);
            inventoryUI.gameObject.SetActive(false);
            AudioManager.Instance.Play("ChestClose");
        }
    }

    public bool IsSetSecondaryInventoryHolder(GameObject inventoryHolder)
    {
        return secondaryInventoryUI.IsSetInventoryHolder(inventoryHolder);
    }

    void Update()
    {
        if (Input.GetKeyDown(KeyCode.I))
        {
            if (!inventoryUI.gameObject.activeInHierarchy)
            {
                ControlModeManager.Instance.SetControlMode(ControlMode.InventoryMode);
            }
            else
            {
                ControlModeManager.Instance.SetControlMode(ControlMode.NormalMode);
            }
            inventoryUI.gameObject.SetActive(!inventoryUI.gameObject.activeInHierarchy);
        }
    }
}
