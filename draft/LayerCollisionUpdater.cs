using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Tilemaps;

public class LayerCollisionUpdater : MonoBehaviour
{
    public int startingLayer = 0;
    public List<int> colliderLayers = new();

    public void TurnOffLayerCollision(int layer)
    {
        if (!colliderLayers.Contains(layer))
        {
            return;
        }
        Physics2D.IgnoreLayerCollision(gameObject.layer, layer, true);
    }

    public void UpdateLayerCollision(int layer)
    {
        if (!colliderLayers.Contains(layer))
        {
            return;
        }
        foreach (int ignoredLayer in colliderLayers)
        {
            Physics2D.IgnoreLayerCollision(gameObject.layer, ignoredLayer, true);
        }
        Physics2D.IgnoreLayerCollision(gameObject.layer, layer, false);
    }
    void Start()
    {
        UpdateLayerCollision(startingLayer);
        enabled = false;
    }
}
