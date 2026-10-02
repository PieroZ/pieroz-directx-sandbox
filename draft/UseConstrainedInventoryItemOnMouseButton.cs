using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class UseConstrainedInventoryItemOnMouseButton : MonoBehaviour
{
    [SerializeField] private int mouseButton = 0;
    public ConstrainedInventory usableItemInventory;
    private Dictionary<ItemType, List<Func<bool>>> conditionSets = new();
    private Dictionary<ItemType, List<Action>> effectorSets = new();
    private List<Func<bool>> currentConditionSet = null;
    private List<Action> currentEffectorSet = null;
    private IFocusable focusable = null;
    private HashSet<Collider2D> collisions = new();

    private void AddCondition(ItemType itemType, Func<bool> condition)
    {
        if (conditionSets.ContainsKey(itemType))
        {
            conditionSets[itemType].Add(condition);
        }
    }

    private bool CheckAllConditions()
    {
        Debug.Log("Check conditions;");
        if (currentConditionSet == null)
        {
            Debug.Log("Check conditions; false");
            return false;
        }    
        foreach (var condition in currentConditionSet)
        {
            if (!condition())
            {
                Debug.Log("Check conditions; false");
                return false;
            }
        }
        Debug.Log("Check conditions; true");
        return true;
    }

    private void ExecuteAllEffectors()
    {
        Debug.Log("ExecuteAllEffectors");
        if (currentEffectorSet == null)
        {
            Debug.Log("ExecuteAllEffectors dupa");
            return;
        }
        foreach (var effector in currentEffectorSet)
        {
            Debug.Log("ExecuteAllEffectors x");
            effector();
        }
    }

    private bool IsCollisionAnEnemy(Collider2D collision)
    {
        EnemyController enemyController;
        collision.TryGetComponent(out enemyController);
        return enemyController != null;
    }

    private bool IsCollisionFocused(Collider2D collision)
    {
        Focusable focusable;
        collision.TryGetComponent(out focusable);
        return focusable != null && focusable.IsFocused();
    }

    private bool IsCollisionAFocusedEnemy(Collider2D collision)
    {
        return IsCollisionAnEnemy(collision) && IsCollisionFocused(collision);
    }

    private bool IsCollidingAnyFocusedEnemy()
    {
        foreach (Collider2D collision in collisions)
        {
            if (IsCollisionAFocusedEnemy(collision))
            {
                return true;
            }
        }
        return false;
    }

    private void Start()
    {
        if (usableItemInventory != null)
        {
            usableItemInventory.OnInventoryChanged += OnItemChanged;
            OnItemChanged();
        }
        if (FocusManager.Instance != null)
        {
            FocusManager.Instance.OnFocusGranted += OnFocusGranted;
            FocusManager.Instance.OnFocusLost += OnFocusLost;
        }

        conditionSets.Add(ItemType.Weapon, new());
        conditionSets[ItemType.Weapon].Add(() => { return IsCollidingAnyFocusedEnemy() && Input.GetMouseButton(mouseButton); });
        effectorSets.Add(ItemType.Weapon, new());
        effectorSets[ItemType.Weapon].Add(() => { Debug.Log("Jest i efekt"); });
        enabled = false;
    }

    private void OnFocusLost()
    {
        focusable = null;
    }

    private void OnFocusGranted()
    {
        focusable = FocusManager.Instance.GetFocus();
    }

    private void OnTriggerEnter2D(Collider2D collision)
    {
        collisions.Add(collision);

        if (usableItemInventory.Get() != null && CheckAllConditions())
        {
            ExecuteAllEffectors();
            Debug.Log("Try use the item: " + usableItemInventory.Get().GetItemName());
        }
    }

    private void OnTriggerExit2D(Collider2D collision)
    {
        collisions.Remove(collision);
    }

    private void OnDestroy()
    {
        if (usableItemInventory != null)
        {
            usableItemInventory.OnInventoryChanged -= OnItemChanged;
        }
        if (FocusManager.Instance != null)
        {
            FocusManager.Instance.OnFocusGranted -= OnFocusGranted;
            FocusManager.Instance.OnFocusLost -= OnFocusLost;
        }
    }

    private void OnItemChanged()
    {
        if (usableItemInventory.Get() == null)
        {
            enabled = false;
            currentConditionSet = null;
            currentEffectorSet = null;
        }
        else
        {
            enabled = true;
            if (conditionSets.ContainsKey(usableItemInventory.Get().GetItemType()))
            {
                currentConditionSet = conditionSets[usableItemInventory.Get().GetItemType()];
                currentEffectorSet = effectorSets[usableItemInventory.Get().GetItemType()];
            }
            else
            {
                currentConditionSet = null;
                currentEffectorSet = null;
            }
        }    
    }
}
