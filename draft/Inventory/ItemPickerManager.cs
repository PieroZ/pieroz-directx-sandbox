using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class ItemPickerManager : MonoBehaviour
{
    private List<ItemPicker> itemPickers = new();
    private Camera mainCamera;
    private Dictionary<Vector3Int, List<ItemPicker>> dormantItemPickers = new();

    public static ItemPickerManager Instance { get; private set; }

    void Awake()
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

    public IEnumerator UpdateActiveInHierarchy()
    {
        while (true)
        {
            if (WorldMapManager.Instance == null)
            {
                break;
            }
            for (int i = 0; i < itemPickers.Count; i++)
            {
                ItemPicker itemPicker = itemPickers[i];
                if (((Vector2)itemPicker.transform.position - (Vector2)mainCamera.transform.position).magnitude > 10)
                {
                    itemPickers.RemoveAt(i);
                    Vector3Int tilePosition = WorldMapManager.Instance.WorldToTilemapCoord(itemPicker.transform.position);
                    tilePosition.z = 0;
                    if (!dormantItemPickers.ContainsKey(tilePosition))
                    {
                        dormantItemPickers.Add(tilePosition, new());
                    }
                    dormantItemPickers[tilePosition].Add(itemPicker);
                    itemPicker.gameObject.SetActive(false);
                    i--; // To compensate for the removed element's 
                }
            }
            yield return new WaitForSeconds(5);
        }
    }

    private void Start()
    {
        mainCamera = Camera.main;
        StartCoroutine(UpdateActiveInHierarchy());
    }

    private void OnDestroy()
    {
        StopAllCoroutines();
    }

    public void RegisterItemPicker(ItemPicker itemPicker)
    {
        itemPickers.Add(itemPicker);
        itemPicker.transform.parent = transform;
    }
    public void UnregisterItemPicker(ItemPicker itemPicker)
    {
        if (itemPickers.Contains(itemPicker))
        {
            itemPickers.Remove(itemPicker);
        }
        if (WorldMapManager.Instance != null)
        {
            Vector3Int tilePosition = WorldMapManager.Instance.WorldToTilemapCoord(itemPicker.transform.position);
            if (dormantItemPickers.ContainsKey(tilePosition) && dormantItemPickers[tilePosition].Contains(itemPicker))
            {
                dormantItemPickers[tilePosition].Remove(itemPicker);
                if (dormantItemPickers[tilePosition].Count <= 0)
                {
                    dormantItemPickers.Remove(tilePosition);
                }
            }
        }
    }

    internal void ActivateDormantItemPickers(Vector3Int tilePosition)
    {
        tilePosition.z = 0;
        if (dormantItemPickers.ContainsKey(tilePosition))
        {
            foreach (ItemPicker itemPicker in dormantItemPickers[tilePosition])
            {
                itemPicker.gameObject.SetActive(true);
                itemPickers.Add(itemPicker);
            }
            dormantItemPickers.Remove(tilePosition);
        }
    }
}
