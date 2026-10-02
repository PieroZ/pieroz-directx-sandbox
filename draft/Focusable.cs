using UnityEngine;
using UnityEngine.EventSystems;

public class Focusable : MonoBehaviour, IFocusable
{
    public delegate void FocusGrantedHandler();
    public FocusGrantedHandler FocusGranted;

    public delegate void FocusLostHandler();
    public FocusLostHandler FocusLost;

    private void OnMouseDown()
    {
        if (!EventSystem.current.IsPointerOverGameObject())
        {
            FocusManager.Instance.SetFocus(this);
            FocusGranted?.Invoke();
        }
    }

    public Vector3 getPosition()
    {
        return transform.position;
    }

    public void OnFocusLost()
    {
        FocusLost?.Invoke();
    }

    public bool IsFocused()
    {
        return FocusManager.Instance.IsFocused(this);
    }

    private void OnDestroy()
    {
        FocusLost?.Invoke();
        FocusManager.Instance.ReleaseFocusImmediately(this);
    }

    public GameObject GetGameObject()
    {
        return gameObject;
    }
}
