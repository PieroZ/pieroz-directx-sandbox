using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class Projectile : MonoBehaviour
{
    public int minDamage = 0;
    public int maxDamage = 0;
    public float speed = 4f;
    public Vector2 direction;
    public bool antiPlayer = false;
    [SerializeField] private GameObject visual;
    private Camera mainCamera;

    private void Awake()
    {
        mainCamera = Camera.main;
    }

    private void Start()
    {
        EntityAudioManager.Instance.Play("Sword1", gameObject);
    }

    public void SetDirection(Vector2 direction)
    {
        this.direction = direction;
        float angle = Mathf.Atan2(direction.y, direction.x) * Mathf.Rad2Deg;
        transform.rotation = Quaternion.AngleAxis(angle, Vector3.forward);
    }

    private void Update()
    {
        if (((Vector2)mainCamera.transform.position - (Vector2)transform.position).magnitude > 16f)
        {
            Destroy(gameObject);
        }
    }

    private void FixedUpdate()
    {
        transform.Translate(speed * Time.deltaTime * Vector2.right);
    }

    protected virtual void DestroyProjectile(Collider2D collision)
    {
        Destroy(gameObject);
    }

    private void OnTriggerEnter2D(Collider2D collision)
    {
        if (!antiPlayer && collision.gameObject.TryGetComponent(out Damageable damageable))
        {
            damageable.Damage(Random.Range(minDamage, maxDamage));
            DestroyProjectile(collision);
        }
        else if (antiPlayer && collision.gameObject.TryGetComponent(out PlayerController _) && collision.gameObject.TryGetComponent(out CharacterHealth characterHealth))
        {
            characterHealth.ApplyDamage(Random.Range(minDamage, maxDamage));
            DestroyProjectile(collision);
        }
        else if (collision.gameObject.layer == (int)transform.position.z / 3 + 13 && collision.gameObject.TryGetComponent(out ColliderLayerInit colliderLayerInit) && colliderLayerInit.offset == 0)
        {
            DestroyProjectile(collision);
        }
    }
}
