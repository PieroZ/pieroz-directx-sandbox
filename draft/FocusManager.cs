using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.EventSystems;

public class FocusManager : MonoBehaviour {

    private IFocusable focus = null;
    private IFocusable nextFocus = null;
    private bool setFocusCalledThisFrame = false;

    public delegate void OnFocusGrantedHandler();
    public OnFocusGrantedHandler OnFocusGranted;

    public delegate void OnFocusLostHandler();
    public OnFocusLostHandler OnFocusLost;

    public static FocusManager Instance { get; private set; }

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
    }

    public void SetFocus(IFocusable focusableObject)
    {
        setFocusCalledThisFrame = true;
        if (IsFocused(focusableObject))
        {
            if (!ReferenceEquals(nextFocus, focus))
            {
                nextFocus = focus;
            }
            return;
        }
        nextFocus = focusableObject;
    }
    public IFocusable GetFocus()
    {
        return focus;
    }

    public bool IsFocused(GameObject gameObject)
    {
        if (gameObject.TryGetComponent(out Focusable focusable))
        {
            return focusable.IsFocused();
        }
        return false;
    }

    public bool IsFocused(IFocusable focusableObject)
    {
        return ReferenceEquals(focus, focusableObject);
    }

    public bool IsFocused()
    {
        return focus != null;
    }

    public void ReleaseFocus(IFocusable focusableObject)
    {
        if (IsFocused(focusableObject))
        {
            if (ReferenceEquals(nextFocus, focusableObject))
            {
                nextFocus = null;
            }
        }
    }
    public void ReleaseFocusImmediately(IFocusable focusableObject)
    {
        if (IsFocused(focusableObject))
        {
            if (ReferenceEquals(nextFocus, focus))
            {
                nextFocus = null;
            }
            focus.OnFocusLost();
            focus = null;
            OnFocusLost?.Invoke();
        }
    }

    void LateUpdate()
    {
        if (!ReferenceEquals(nextFocus, focus))
        {
            if (focus != null)
            {
                focus.OnFocusLost();
            }
            focus = nextFocus;
            if (focus == null)
            {
                OnFocusLost?.Invoke();
            }
            else
            {
                OnFocusGranted.Invoke();
            }
        }
#if UNITY_ANDROID
        else if (Input.touchCount > 0 && Input.GetTouch(0).phase == TouchPhase.Began && !EventSystem.current.IsPointerOverGameObject(Input.GetTouch(0).fingerId) && !setFocusCalledThisFrame && !FloatingItemPickerManagerUI.Instance.IsFloating())
#else
        else if (Input.GetMouseButtonDown(0) && !EventSystem.current.IsPointerOverGameObject() && !setFocusCalledThisFrame && !FloatingItemPickerManagerUI.Instance.IsFloating())
#endif
        {
            ReleaseFocus(focus);
        }
        setFocusCalledThisFrame = false;
    }
}
