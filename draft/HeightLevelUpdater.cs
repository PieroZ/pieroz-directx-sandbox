using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Tilemaps;

public class HeightLevelUpdater : MonoBehaviour
{
    private void Start()
    {
        enabled = false;
    }
    private void UpdateHeightLevelOnEntry(Collider2D other)
    {
        if (other == null || !(other.gameObject.CompareTag("Player") || other.gameObject.CompareTag("NPC")))
        {
            return;
        }

        if (other.gameObject.CompareTag("Player"))
        {
            if (other.TryGetComponent(out LayerCollisionUpdater layerCollisionUpdater))
            {
                other.transform.position = new Vector3(other.transform.position.x, other.transform.position.y, transform.position.z + 4);
                layerCollisionUpdater.TurnOffLayerCollision((int)(transform.position.z / 3f + 14f));
                layerCollisionUpdater.TurnOffLayerCollision((int)(transform.position.z / 3f + 13f));
            }
        }
        else if (other.gameObject.CompareTag("NPC"))
        {

        }
    }

    private void UpdateHeightLevelOnExit(Collider2D other)
    {
        if (other == null || !(other.gameObject.CompareTag("Player") || other.gameObject.CompareTag("NPC")))
        {
            return;
        }
        
        if (other.gameObject.CompareTag("Player"))
        {
            if (other.TryGetComponent(out LayerCollisionUpdater layerCollisionUpdater))
            {
                if (other.TryGetComponent(out PlayerController playerController))
                {
                    if (playerController.GetMovementDirection().y > 0)
                    {
                        layerCollisionUpdater.UpdateLayerCollision((int)(transform.position.z / 3f + 14f));
                        other.transform.position = new Vector3(other.transform.position.x, other.transform.position.y, transform.position.z + 4);
                    }
                    else
                    {
                        layerCollisionUpdater.UpdateLayerCollision((int)(transform.position.z / 3f + 13f));
                        other.transform.position = new Vector3(other.transform.position.x, other.transform.position.y, transform.position.z + 1);
                    }
                }
            }
        }
        else if (other.gameObject.CompareTag("NPC"))
        {

        }
    }

    private void OnTriggerEnter2D(Collider2D other)
    {
        if (!other.isTrigger)
        {
            return;
        }
        UpdateHeightLevelOnEntry(other);
    }

    private void OnTriggerExit2D(Collider2D other)
    {
        if (!other.isTrigger)
        {
            return;
        }
        UpdateHeightLevelOnExit(other);
    }

}
