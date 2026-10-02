using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public enum ControlMode
{
    NotAMode,
    NormalMode,
    InventoryMode
}

public class ControlModeManager : MonoBehaviour
{
    public ControlMode controlMode = ControlMode.NormalMode;
    private ControlMode nextControlMode = ControlMode.NotAMode;

    public static ControlModeManager Instance { get; private set; }

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

    public void SetControlMode(ControlMode mode)
    {
        nextControlMode = mode;
    }

    void LateUpdate()
    {
        if (nextControlMode != ControlMode.NotAMode)
        {
            controlMode = nextControlMode;
            nextControlMode = ControlMode.NotAMode;
        }
    }
}
