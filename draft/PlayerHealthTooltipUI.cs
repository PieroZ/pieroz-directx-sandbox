using System.Collections;
using UnityEngine;
using TMPro;
using UnityEngine.UI;

public class PlayerHealthTooltipUI : MonoBehaviour
{
    public TMP_Text healthText;
    public Image background;
    public Image healthBar;
    public CharacterHealth characterHealth = null;

    private void Start()
    {
        if (characterHealth == null)
        {
            PlayerManager.Instance.GetPlayerController().TryGetComponent(out characterHealth);
            characterHealth.OnDamaged += UpdateItem;
            UpdateItem();
        }
    }

    public void UpdateItem()
    {
        healthText.text = characterHealth.GetCurrentHealth().ToString();
        background.GetComponent<RectTransform>().sizeDelta = new Vector2(healthText.GetComponent<RectTransform>().rect.width, healthText.preferredHeight);
        healthBar.GetComponent<RectTransform>().sizeDelta = background.GetComponent<RectTransform>().sizeDelta;
        healthBar.transform.position = background.transform.position;
        healthBar.GetComponent<RectTransform>().sizeDelta = new Vector2(healthText.GetComponent<RectTransform>().rect.width * (float)characterHealth.GetCurrentHealth() / (float)characterHealth.GetMaxHealth(), healthText.preferredHeight);
        healthText.transform.position = background.transform.position;
    }

    private void OnDestroy()
    {
        if (characterHealth != null)
        {
            characterHealth.OnDamaged -= UpdateItem;
        }
    }

}
