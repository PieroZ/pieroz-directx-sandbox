using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class Explodeable : MonoBehaviour
{
    public Animator animationPrefab;
    public int blastRadius = 1;
    public int blastDamage = 50;
    public float fuseTime = 3f;
    IEnumerator Fuse()
    {
        yield return new WaitForSeconds(fuseTime);
        Explode();
    }

    void Start()
    {
        AudioManager.Instance.Play("BombDrop");
        StartCoroutine(Fuse());
    }
    private void Explode()
    {
        EntityAudioManager.Instance.Play("BombExplosion", gameObject);
        if (animationPrefab != null)
        {
            Instantiate(animationPrefab, transform.position, Quaternion.identity, transform.parent);
        }
        Collider2D[] colliders = Physics2D.OverlapCircleAll(transform.position, blastRadius);
        foreach (Collider2D collider in colliders)
        {
            if (collider.TryGetComponent(out Damageable damageable))
            {
                damageable.Damage(blastDamage);
            }
        }

        if (WorldMapManager.Instance != null)
        {
            WorldMapManager.Instance.DestroyTilesInRadius(transform.position, blastRadius);
        }
        float destroyDelay = 0f;
        if (TryGetComponent(out AudioSource audioSource))
        {
            destroyDelay = audioSource.clip.length;
        }
        if (TryGetComponent(out SpriteRenderer spriteRenderer))
        {
            spriteRenderer.enabled = false;
        }
        Destroy(gameObject, destroyDelay);
    }
}
