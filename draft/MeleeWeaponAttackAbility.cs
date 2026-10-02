using System.Collections;
using UnityEngine;

public class MeleeWeaponAttackAbility : MonoBehaviour
{
    private int minDamage = 1;
    private int maxDamage = 1;
    private float attackSpeed = 0.75f;
    private bool meleeWeapon = false;
    private CharacterStatistics characterStatistics = null;
    public ConstrainedInventory[] weaponInventories;
    private bool currentlyAttacking = false;
    public bool IsAttacking()
    {
        return currentlyAttacking;
    }

    public bool CanAttack()
    {
        return meleeWeapon;
    }

    private void Start()
    {
        if (TryGetComponent(out characterStatistics))
        {
            characterStatistics.OnStatisticsChanged += OnStatisticsChanged;
            OnStatisticsChanged();
        }
        
        foreach (ConstrainedInventory weaponInventory in weaponInventories)
        {
            weaponInventory.OnInventoryChanged += OnWeaponChanged;
        }
    }

    private void RecalculateDamage()
    {
        attackSpeed = 0.75f;
        if (characterStatistics != null)
        {
            minDamage = characterStatistics.basePrimaryStatistics.Get(PrimaryStatistics.Strength);
            maxDamage = characterStatistics.basePrimaryStatistics.Get(PrimaryStatistics.Strength);
        }
        foreach (ConstrainedInventory weaponInventory in weaponInventories)
        {
            Item weapon = weaponInventory.Get();
            if (weapon != null)
            {
                meleeWeapon = ItemType.Weapon == weapon.GetItemType();
                if (weapon.scriptableItem.additionalAttributes.ContainsKey(ItemAdditionalAttributes.MinDamage))
                {
                    minDamage += weapon.scriptableItem.additionalAttributes[ItemAdditionalAttributes.MinDamage];
                }
                if (weapon.scriptableItem.additionalAttributes.ContainsKey(ItemAdditionalAttributes.MaxDamage))
                {
                    maxDamage += weapon.scriptableItem.additionalAttributes[ItemAdditionalAttributes.MaxDamage];
                }
                if (weapon.scriptableItem.additionalAttributes.ContainsKey(ItemAdditionalAttributes.AttackRate))
                {
                    attackSpeed = 1f / weapon.scriptableItem.additionalAttributes[ItemAdditionalAttributes.AttackRate];
                }
            }
            else
            {
                meleeWeapon = true;
            }
        }
    }

    private void OnWeaponChanged()
    {
        RecalculateDamage();
    }

    private void OnStatisticsChanged()
    {
        RecalculateDamage();
    }

    private IEnumerator Attack(CharacterHealth characterHealth)
    {
        currentlyAttacking = true;
        while (characterHealth.IsAlive())
        {
            characterHealth.ApplyDamage((int)Random.Range(minDamage, maxDamage));
            AudioManager.Instance.Play("Sword" + Random.Range(1, 4));
            yield return new WaitForSeconds(attackSpeed);
        }
        currentlyAttacking = false;
    }

    private IEnumerator Attack(Damageable damageable)
    {
        currentlyAttacking = true;
        bool atLeastOnceAttacked = false;
        while (damageable.IsFocused())
        {
            if (Input.GetMouseButton(0) || !atLeastOnceAttacked)
            {
                damageable.Damage(Random.Range(minDamage, maxDamage));
                atLeastOnceAttacked = true;
                AudioManager.Instance.Play("Sword" + Random.Range(1, 4));
                yield return new WaitForSeconds(attackSpeed);
            }
            else
            {
                yield return new WaitForEndOfFrame();
                yield return new WaitForEndOfFrame();
            }
        }
        currentlyAttacking = false;
    }

    public void StartAttack(CharacterHealth characterHealth)
    {
        if (meleeWeapon && !currentlyAttacking)
        {
            StartCoroutine(Attack(characterHealth));
        }
    }

    public void StartAttack(Damageable damageable)
    {
        if (meleeWeapon && !currentlyAttacking)
        {
            StartCoroutine(Attack(damageable));
        }
    }

    public void StopAttack()
    {
        StopAllCoroutines();
        currentlyAttacking = false;
    }

    private void OnDestroy()
    {
        if (characterStatistics != null)
        {
            characterStatistics.OnStatisticsChanged -= OnStatisticsChanged;
        }
        foreach (ConstrainedInventory weaponInventory in weaponInventories)
        {
            weaponInventory.OnInventoryChanged -= OnWeaponChanged;
        }
    }
}
