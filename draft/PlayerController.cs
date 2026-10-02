using System.Collections.Generic;
using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.SceneManagement;

public class PlayerController : MonoBehaviour
{
    public float speed = 2f;
    private Vector2 lastClickedPos;

    private Rigidbody2D rb;
    private Animator anim;

    private const float distanceEpsilon = 0.01f;
    private bool focusedMovementMode = false;
    private MeleeWeaponAttackAbility meleeWeaponAttackAbility = null;
    private CharacterHealth characterHealth = null;
    private Damageable targetDamageable = null;
    private bool upperAttackHysteresis = true;

    Vector3 oldPosition;
    Vector2 fixedUpdateOldPosition;
    private bool moving = false;
    private bool fixedUpdateMoving = false;
    Vector2 movementDirection = Vector2.zero;

    void Start()
    {
        rb = GetComponent<Rigidbody2D>();
        anim = GetComponentInChildren<Animator>();

        TryGetComponent(out meleeWeaponAttackAbility);
        if (TryGetComponent(out characterHealth))
        {
            characterHealth.OnDied += OnDied;
        }

        lastClickedPos = transform.position;
        oldPosition = transform.position;

        FocusManager.Instance.OnFocusGranted += OnFocusGranted;
        FocusManager.Instance.OnFocusLost += OnFocusLost;
    }

    public Vector2 GetMovementDirection()
    {
        return transform.position - oldPosition;
    }

    void OnDied()
    {
        if (characterHealth != null)
        {
            characterHealth.SetCurrentHealth(characterHealth.GetMaxHealth());
        }
    }
    private void Update()
    {
        movementDirection = Vector2.zero;
        if (Input.GetKey(KeyCode.W))
        {
            movementDirection += Vector2.up;
        }
        if (Input.GetKey(KeyCode.S))
        {
            movementDirection += Vector2.down;
        }
        if (Input.GetKey(KeyCode.A))
        {
            movementDirection += Vector2.left;
        }
        if (Input.GetKey(KeyCode.D))
        {
            movementDirection += Vector2.right;
        }

        if (movementDirection != Vector2.zero)
        {
            focusedMovementMode = false;
            movementDirection.Normalize();
            UpdateOrientation();
        }
        else if (focusedMovementMode)
        {
            if (targetDamageable != null)
            {
                if (upperAttackHysteresis || !Input.GetMouseButton(0))
                {
                    lastClickedPos = transform.position;
                }
                else
                {
                    lastClickedPos = FocusManager.Instance.GetFocus().getPosition();
                }
            }
            else
            {
                lastClickedPos = FocusManager.Instance.GetFocus().getPosition();
                upperAttackHysteresis = false;
            }
            UpdateOrientation();
        }
        if (!moving && fixedUpdateMoving)
        {
            moving = true;
            AudioManager.Instance.PlayLoop("Footsteps1");
            anim.SetTrigger("TrRun");
        }
        else if (moving && !fixedUpdateMoving)
        {
            moving = false;
            AudioManager.Instance.StopLoop("Footsteps1");
            anim.SetTrigger("TrIdle");
        }
        oldPosition = transform.position;
    }

    private void FixedUpdate()
    {
        if (focusedMovementMode)
        {
            if (Vector2.Distance(rb.position, lastClickedPos) > distanceEpsilon)
            {
                fixedUpdateMoving = true;
                rb.MovePosition(Vector2.MoveTowards(rb.position, lastClickedPos, speed * Time.deltaTime));
            }
            else
            {
                fixedUpdateMoving = false;
                rb.linearVelocity = Vector3.zero;
                rb.position = lastClickedPos;
            }
            fixedUpdateMoving = fixedUpdateOldPosition != rb.position;
            fixedUpdateOldPosition = rb.position;
        }
        else
        {
            if (movementDirection != Vector2.zero)
            {
                fixedUpdateMoving = true;
                rb.MovePosition(Vector2.MoveTowards(rb.position, rb.position + movementDirection, speed * Time.deltaTime));
            }
            else
            {
                fixedUpdateMoving = false;
                rb.linearVelocity = Vector3.zero;
            }
        }
    }

    private void StartAttack(Collider2D collision)
    {
        if (meleeWeaponAttackAbility != null && meleeWeaponAttackAbility.CanAttack())
        {
            if (focusedMovementMode && collision.TryGetComponent(out Focusable focusable) && focusable.IsFocused())
            {
                if (meleeWeaponAttackAbility.CanAttack() && collision.TryGetComponent(out Damageable damageable))
                {
                    meleeWeaponAttackAbility.StartAttack(damageable);
                }
            }
        }
    }

    void OnFocusGranted()
    {
        focusedMovementMode = true;
        GameObject focusedObject = FocusManager.Instance.GetFocus().GetGameObject();
        if (focusedObject.TryGetComponent(out Damageable damageable))
        {
            targetDamageable = damageable;
        }
        TryAttack();
    }

    void OnFocusLost()
    {
        focusedMovementMode = false;
        targetDamageable = null;
    }

    private void OnTriggerEnter2D(Collider2D collision)
    {
        StartAttack(collision);
    }

    private void TryAttack()
    {
        if (targetDamageable == null)
        {
            return;
        }
        List<Collider2D> colliders = new();
        GetComponent<BoxCollider2D>().Overlap(new ContactFilter2D().NoFilter(), colliders);
        foreach (Collider2D collision in colliders)
        {
            StartAttack(collision);
        }
    }

    private void OnTriggerExit2D(Collider2D collision)
    {
        if (focusedMovementMode && collision.TryGetComponent(out Focusable focusable) && FocusManager.Instance.IsFocused(focusable))
        {
            if (collision.TryGetComponent(out Damageable damageable))
            {
                //Debug.Log("Damagable out!");
            }
        }
    }

    private void UpdateOrientation()
    {
        if (movementDirection.x < distanceEpsilon)
        {
            transform.localScale = Vector3.one;
        }
        else if (movementDirection.x > -distanceEpsilon)
        {
            transform.localScale = new Vector3(-1, 1, 1);
        }
    }

    private void OnDestroy()
    {
        FocusManager.Instance.OnFocusGranted -= OnFocusGranted;
        FocusManager.Instance.OnFocusLost -= OnFocusLost;

        if (characterHealth != null)
        {
            characterHealth.OnDied -= OnDied;
        }
    }
}
