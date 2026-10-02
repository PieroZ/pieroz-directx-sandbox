using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class CharacterHealth : MonoBehaviour
{
    public delegate void OnDamagedHandler();
    public OnDamagedHandler OnDamaged;

    public delegate void OnDiedHandler();
    public OnDiedHandler OnDied;

    [SerializeField] private int currentHealth = 100;
    [SerializeField] private int maxHealth = 100;

    private ArmorDamageReductionAbility armorDamageReductionAbility = null;

    private void Start()
    {
        TryGetComponent(out armorDamageReductionAbility);
    }

    public int GetCurrentHealth()
    {
        return currentHealth;
    }

    public int GetMaxHealth()
    {
        return maxHealth;
    }

    public void SetCurrentHealth(int health)
    {
        currentHealth = health;
        OnDamaged?.Invoke();
    }

    public void SetMaxHealth(int health)
    {
        maxHealth = health;
        OnDamaged?.Invoke();
    }

    public void Heal(int health)
    {
        currentHealth = Mathf.Min(currentHealth + health, maxHealth);
        OnDamaged?.Invoke();
    }

    public void ApplyDamage(int damage)
    {
        if (currentHealth <= 0 || damage < 0)
        {
            return;
        }

        if (armorDamageReductionAbility)
        {
            damage -= armorDamageReductionAbility.GetDamageReduction();
            if (damage < 1)
            {
                damage = 1;
            }
        }
        currentHealth -= damage;

        if (currentHealth <= 0)
        {
            currentHealth = 0;
            OnDamaged?.Invoke();
            OnDied?.Invoke();
        }
        else
        {
            OnDamaged?.Invoke();
        }
    }

    public bool IsAlive()
    {
        return currentHealth > 0;
    }

    public bool IsDead()
    {
        return !IsAlive();
    }
}
