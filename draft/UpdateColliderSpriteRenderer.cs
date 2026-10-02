using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.Rendering.Universal;
using UnityEngine.Tilemaps;

public class UpdateColliderSpriteRenderer : MonoBehaviour
{
    private void Start()
    {
        GetComponent<SpriteRenderer>().sprite = GetComponentInParent<Tilemap>().GetSprite(GetComponentInParent<Tilemap>().WorldToCell(transform.position));
        enabled = false;
    }
}
