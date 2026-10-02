using UnityEngine;
using UnityEngine.EventSystems;

public class InteractableTooltipUIWrapper : MonoBehaviour
{
    public InteractableTooltipUI interactableTooltipUIPrefab;
    private InteractableTooltipUI interactableTooltipUI = null;
    public string interactableName = "defaultName";
    private bool forceTooltipVisibility = false;
    private bool mouseOver = false;
    private bool isSelected = false;

    public void ForceMouseSensitivity()
    {
        if (interactableTooltipUI != null)
        {
            interactableTooltipUI.ForceMouseSensitivity();
        }
    }

    public void UnforceMouseSensitivity()
    {
        if (interactableTooltipUI != null)
        {
            interactableTooltipUI.UnforceMouseSensitivity();
        }
    }

    public Vector3 GetPosition()
    {
        if (interactableTooltipUI != null)
        {
            return interactableTooltipUI.GetPosition();
        }
        return Vector3.zero;
    }

    public void Select()
    {
        if (interactableTooltipUI != null)
        {
            interactableTooltipUI.Select();
        }
        isSelected = true;
    }

    public void Deselect()
    {
        if (interactableTooltipUI != null)
        {
            interactableTooltipUI.Deselect();
        }
        isSelected = false;
    }

    private void Awake()
    {
        enabled = false;
    }

    public void ForceTooltipVisibility()
    {
        forceTooltipVisibility = true;
        if (interactableTooltipUI == null)
        {
            interactableTooltipUI = Instantiate(interactableTooltipUIPrefab, transform);
        }
        interactableTooltipUI.gameObject.SetActive(true);
        interactableTooltipUI.UpdateItem(this);
        interactableTooltipUI.ForceMouseSensitivity();
        if (isSelected)
        {
            interactableTooltipUI.Select();
        }
        enabled = true;
    }

    public void UnforceTooltipVisibility()
    {
        forceTooltipVisibility = false;
        if (interactableTooltipUI && !mouseOver)
        {
            interactableTooltipUI.UnforceMouseSensitivity();
            interactableTooltipUI.gameObject.SetActive(false);
            enabled = false;
        }
    }

    private void OnMouseEnter()
    {
        if (!EventSystem.current.IsPointerOverGameObject())
        {
            mouseOver = true;
            if (interactableTooltipUI == null)
            {
                interactableTooltipUI = Instantiate(interactableTooltipUIPrefab, transform);
            }
            interactableTooltipUI.gameObject.SetActive(true);
            interactableTooltipUI.UpdateItem(this);
            if (isSelected)
            {
                interactableTooltipUI.Select();
            }
            enabled = true;
        }
    }

    private void OnMouseExit()
    {
        mouseOver = false;
        if (interactableTooltipUI && !forceTooltipVisibility)
        {
            interactableTooltipUI.gameObject.SetActive(false);
            enabled = false;
        }
    }

    private void OnDestroy()
    {
        if (interactableTooltipUI)
        {
            Destroy(interactableTooltipUI.gameObject);
        }
    }
}
