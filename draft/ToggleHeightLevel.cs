using UnityEngine;
using UnityEngine.Tilemaps;

public class ToggleHeightLevel : MonoBehaviour
{
    public float height = 0.0f;
    public int layer = 0;
    private void Start()
    {
        enabled = false;
    }
    
    private void UpdateHeightLevel(Collider2D other)
    {
        var go = other.gameObject;
        if (go == null || !(other.gameObject.CompareTag("Player") || other.gameObject.CompareTag("NPC")) || go.transform.position.z == height)
        {
            return;
        }
        if (other.gameObject.CompareTag("Player"))
        {
            if (other.TryGetComponent(out LayerCollisionUpdater layerCollisionUpdater))
            {
                layerCollisionUpdater.UpdateLayerCollision(layer);
            }
        }
        else if (other.gameObject.CompareTag("NPC"))
        {

        }

        var position = go.transform.position;
        var newPosition = new Vector3(position.x, position.y, height);
        go.transform.position = newPosition;
    }

    private void OnTriggerEnter2D(Collider2D other)
    {
        UpdateHeightLevel(other);
    }
}
