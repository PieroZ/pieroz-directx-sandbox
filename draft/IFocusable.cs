using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public interface IFocusable
{
    public abstract void OnFocusLost();
    public abstract Vector3 getPosition();
    public abstract GameObject GetGameObject();
}
