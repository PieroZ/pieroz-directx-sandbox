using UnityEngine;
using UnityEngine.Tilemaps;

public class HideTilemapColliderOnPlay : MonoBehaviour
{
    void Start()
    {
        TilemapRenderer tilemapRenderer = GetComponent<TilemapRenderer>();
        tilemapRenderer.enabled = false;
        enabled = false;
    }
}
