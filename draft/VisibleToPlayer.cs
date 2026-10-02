using UnityEngine;

public class VisibleToPlayer : MonoBehaviour
{
    public delegate void OnSpottedByPlayerHandler();
    public OnSpottedByPlayerHandler OnSpottebByPlayer;

    public delegate void OnSightLostByPlayerHandler();
    public OnSightLostByPlayerHandler OnSightLostByPlayer;

    public LayerMask blockingLayer;
    [SerializeField] public float visibilityRange = 6.4f;

    private bool visible = false;
    private RaycastHit2D hit;
    private Vector2 direction;


    private void UpdateVisible()
    {
        bool isVisible = CalculateVisible();
        if (isVisible && !visible)
        {
            OnSpottebByPlayer?.Invoke();
            visible = true;
        }
        else if (!isVisible && visible)
        {
            OnSightLostByPlayer?.Invoke();
            visible = false;
        }
    }
    public bool IsVisible()
    {
        UpdateVisible();
        return visible;
    }

    private void FixedUpdate()
    {
        UpdateVisible();
    }

    private bool CalculateVisible()
    {
        if (!PlayerManager.Instance.IsPlayerControllerAvailable())
        {
            return false;
        }
        direction = PlayerManager.Instance.GetPlayerController().transform.position - transform.position;
        if (direction.magnitude > visibilityRange)
        {
            return false;
        }
        if (direction.magnitude < .16f)
        {
            return true;
        }
        Physics2D.queriesHitTriggers = false;
        hit = Physics2D.Raycast(transform.position, direction, visibilityRange, blockingLayer);
        Physics2D.queriesHitTriggers = true;
        /* //debug lines
        if (hit.collider == null)
        {
            return false;
        }

        if (hit.collider.gameObject == PlayerManager.Instance.GetPlayerController().gameObject)
        {
            Debug.DrawLine(hit.point, transform.position, Color.red);
        }
        else
        {
            Debug.DrawLine(hit.point, transform.position, Color.white);
        }//*/

        return hit.collider != null && hit.collider.gameObject == PlayerManager.Instance.GetPlayerController().gameObject;
    }
}


