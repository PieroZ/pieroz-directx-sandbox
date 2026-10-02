using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Rendering.Universal;

public class TurnOffShadowColliderWhenOutOfScreen : MonoBehaviour
{
    private Transform playerTransform;
    private ShadowCaster2D shadowCaster2D;
    private float epsilon = 3.2f;
    private CircleCollider2D circleCollider2D;

    private void Awake()
    {
        if (!TryGetComponent(out circleCollider2D))
        {
            circleCollider2D = gameObject.AddComponent(typeof(CircleCollider2D)) as CircleCollider2D;
            circleCollider2D.isTrigger = true;
            circleCollider2D.radius = epsilon;
        }
    }

    private void Start()
    {
        playerTransform = (FindFirstObjectByType(typeof(PlayerController)) as PlayerController).transform;
        if (playerTransform == null)
        {
            return;
        }
        shadowCaster2D = GetComponent<ShadowCaster2D>();
        shadowCaster2D.enabled = (transform.position - playerTransform.position).magnitude < epsilon;
    }

    private void OnTriggerEnter2D(Collider2D collision)
    {
        if (playerTransform == null)
        {
            return;
        }
        shadowCaster2D.enabled = (transform.position - playerTransform.position).magnitude < epsilon;
    }

    private void OnTriggerExit2D(Collider2D collision)
    {
        if (playerTransform == null)
        {
            return;
        }
        shadowCaster2D.enabled = (transform.position - playerTransform.position).magnitude < epsilon;
    }

}
