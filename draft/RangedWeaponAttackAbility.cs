using System.Collections;
using UnityEngine;

public class RangedWeaponAttackAbility : MonoBehaviour
{
    private int minDamage = 1;
    private int maxDamage = 1;
    private int range = 1;
    private float attackSpeed = 0.75f;
    public float attackRateLimit = float.MaxValue;
    private bool rangedWeapon = false;
    private CharacterStatistics characterStatistics = null;
    public ConstrainedInventory[] weaponInventories;
    public Projectile projectilePrefab;
    private bool currentlyAttacking = false;

    public bool IsAttacking()
    {
        return currentlyAttacking;
    }

    public bool CanAttack()
    {
        return rangedWeapon;
    }

    public bool CanAttack(Damageable damageable, float rangeModifier = 1.0f)
    {
        return CanAttack() && (((Vector2)damageable.transform.position - (Vector2)transform.position).magnitude <= range * .32f * rangeModifier);
    }

    public bool CanAttack(CharacterHealth characterHealth, float rangeModifier = 1.0f)
    {
        return CanAttack() && (((Vector2)characterHealth.transform.position - (Vector2)transform.position).magnitude <= range * .32f * rangeModifier);
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
        range = 1;
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
                rangedWeapon = ItemType.RangedWeapon == weapon.GetItemType();
                if (weapon.scriptableItem.additionalAttributes.ContainsKey(ItemAdditionalAttributes.MinDamage))
                {
                    minDamage += weapon.scriptableItem.additionalAttributes[ItemAdditionalAttributes.MinDamage];
                }
                if (weapon.scriptableItem.additionalAttributes.ContainsKey(ItemAdditionalAttributes.MaxDamage))
                {
                    maxDamage += weapon.scriptableItem.additionalAttributes[ItemAdditionalAttributes.MaxDamage];
                }
                if (weapon.scriptableItem.additionalAttributes.ContainsKey(ItemAdditionalAttributes.Range))
                {
                    range = weapon.scriptableItem.additionalAttributes[ItemAdditionalAttributes.Range];
                }
                if (weapon.scriptableItem.additionalAttributes.ContainsKey(ItemAdditionalAttributes.AttackRate))
                {
                    attackSpeed = 1f / Mathf.Min(attackRateLimit, weapon.scriptableItem.additionalAttributes[ItemAdditionalAttributes.AttackRate]);
                }
            }
            else
            {
                rangedWeapon = false;
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
        while (characterHealth.IsAlive() && CanAttack(characterHealth) && GetComponent<VisibleToPlayer>().IsVisible())
        {
            Projectile projectile = Instantiate(projectilePrefab, transform.position, Quaternion.identity);
            projectile.SetDirection(characterHealth.transform.position - transform.position);
            projectile.minDamage = minDamage;
            projectile.maxDamage = maxDamage;
            projectile.antiPlayer = true;
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
            if ((Input.GetMouseButton(0) || !atLeastOnceAttacked) && CanAttack(damageable))
            {
                Projectile projectile = Instantiate(projectilePrefab, transform.position, Quaternion.identity);
                projectile.SetDirection(damageable.transform.position - transform.position);

                projectile.minDamage = minDamage;
                projectile.maxDamage = maxDamage;
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
        if (rangedWeapon && !currentlyAttacking)
        {
            StartCoroutine(Attack(characterHealth));
        }
    }

    public void StartAttack(Damageable damageable)
    {
        if (rangedWeapon && !currentlyAttacking)
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
