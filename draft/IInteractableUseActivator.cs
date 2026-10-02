using UnityEngine;

public interface IInteractableUseActivator
{
    public void Use();
    public void Unuse();
    public void Select();
    public void Deselect();
    public Vector3 GetPosition();
}
