using UnityEngine;
using TMPro;
using UnityEngine.UI;
using UnityEngine.EventSystems;

public class InteractableTooltipUI : MonoBehaviour, IPointerEnterHandler, IPointerExitHandler, IPointerDownHandler
{
    public TMP_Text interactableName;
    public Image background;
    private Vector3 position = Vector3.zero;
    private InteractableTooltipUIWrapper interactableObject = null;
    private bool forceMouseSensitivity = false;
    bool selected = false;

    public Vector3 GetPosition()
    {
        return position;
    }

    public void Select()
    {
        selected = true;
        //background.color = new Color(255f, 0f, 0f);
        background.color = Color.red;
    }

    public void Deselect()
    {
        selected = false;
        //background.color = new Color(9f, 9f, 67f);
        ColorUtility.TryParseHtmlString("#090943", out Color color);
        background.color = color;
    }
    public void ForceMouseSensitivity()
    {
        forceMouseSensitivity = true;
        background.raycastTarget = true;
    }

    public void UnforceMouseSensitivity()
    {
        forceMouseSensitivity = false;
    }

    public void UpdateItem(InteractableTooltipUIWrapper interactableObject)
    {
        this.interactableObject = interactableObject;
        interactableName.text = interactableObject.interactableName;
        background.GetComponent<RectTransform>().sizeDelta = new Vector2(interactableName.preferredWidth, interactableName.preferredHeight);
        position = Camera.main.WorldToScreenPoint(interactableObject.transform.position);
        position.x -= interactableName.preferredWidth / 2;
        position.y += interactableName.preferredHeight / 2;
        interactableName.transform.position = position;
        background.transform.position = position;
        background.raycastTarget = forceMouseSensitivity;
    }

    private void Update()
    {
        if (interactableObject != null)
        {
            background.raycastTarget = forceMouseSensitivity;
            position = Camera.main.WorldToScreenPoint(interactableObject.transform.position);
            position.x -= interactableName.preferredWidth / 2;
            position.y += interactableName.preferredHeight / 2;
            interactableName.transform.position = position;
            background.transform.position = position;
        }
    }

    public void OnPointerEnter(PointerEventData eventData)
    {
        if (forceMouseSensitivity)
        {
            InteractableUseManager.Instance.ForceSelect(interactableObject.GetComponent<IInteractableUseActivator>());
        }
        background.color = Color.red;
    }

    public void OnPointerExit(PointerEventData eventData)
    {
        if (!selected)
        {
            ColorUtility.TryParseHtmlString("#090943", out Color color);
            background.color = color;
        }
    }

    public void OnPointerDown(PointerEventData eventData)
    {
        FocusManager.Instance.SetFocus(interactableObject.GetComponent<Focusable>());
        if (forceMouseSensitivity)
        {
            InteractableUseManager.Instance.ForceUse(interactableObject.GetComponent<IInteractableUseActivator>());
        }
    }
}
