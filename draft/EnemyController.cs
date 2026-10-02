using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class EnemyController : MonoBehaviour
{
    public float speed = 1f;
    public GameObject patrolPointsList;
    private List<Vector3> patrolPoints = new();
    private Transform mainTarget = null;
    public float visionDistance = 6.4f;
    private List<Vector3>.Enumerator patrolPointsEnumerator;
    private const float distanceEpsilon = 0.01f;
    private Rigidbody2D rb;
    private Vector2 currentMoveTarget;
    private bool followingMainTarget = false;
    private VisibleToPlayer visibleToPlayer = null;
    private CharacterHealth characterHealth = null;
    private Vector2 oldPosition = Vector2.zero;
    private bool walking;
    private MeleeWeaponAttackAbility meleeWeaponAttackAbility = null;
    private RangedWeaponAttackAbility rangedWeaponAttackAbility = null;
    [SerializeField] public string monsterTypeName = "DefaultMonsterTypeName";
    private Vector3 inverseLocalScale = new Vector3(-1, 1, 1);

    IEnumerator VoiceGenerator()
    {
        while (true)
        {
            yield return new WaitForSeconds(Random.Range(5f,30f));
            EntityAudioManager.Instance.Play("Growl" + Random.Range(1, 8), gameObject);
        }
    }

    private void OnMouseDown()
    {
        EntityAudioManager.Instance.Play("Growl" + Random.Range(1, 8), gameObject);
    }

    private void Start()
    {
        if (mainTarget == null)
        {
            PlayerController playerController = PlayerManager.Instance.GetPlayerController();
            if (playerController != null)
            {
                mainTarget = playerController.transform;
            }
        }
        TryGetComponent(out meleeWeaponAttackAbility);
        TryGetComponent(out rangedWeaponAttackAbility);
        if (TryGetComponent(out visibleToPlayer))
        {
            visibleToPlayer.OnSpottebByPlayer += OnSpottebByPlayer;
            visibleToPlayer.OnSightLostByPlayer += OnSightLostByPlayer;
        }
        if (TryGetComponent(out characterHealth))
        {
            characterHealth.OnDied += OnDied;
            characterHealth.OnDamaged += OnDamaged;
        }
        StartCoroutine(VoiceGenerator());
    }

    void OnDied()
    {
        StopAllCoroutines();
        transform.Rotate(new Vector3(0, 0, 90));
        rb.simulated = false;
        if (visibleToPlayer != null)
        {
            visibleToPlayer.OnSpottebByPlayer -= OnSpottebByPlayer;
            visibleToPlayer.OnSightLostByPlayer -= OnSightLostByPlayer;
            Destroy(visibleToPlayer);
            visibleToPlayer = null;
        }
        if (TryGetComponent(out Focusable focusable))
        {
            if (focusable.IsFocused())
            {
                FocusManager.Instance.ReleaseFocus(focusable);
            }
        }
        Component[] shadowCaster2Ds = GetComponentsInChildren<UnityEngine.Rendering.Universal.ShadowCaster2D>();
        foreach (Component shadowCaster2D in shadowCaster2Ds)
        {
            Destroy(shadowCaster2D);
        }

        if (TryGetComponent(out SpriteRenderer spriteRenderer))
        {
            spriteRenderer.sortingLayerName = "Background";
        }
        EntityAudioManager.Instance.Play("Death" + Random.Range(1, 5), gameObject);
        Animator animator = GetComponentInChildren<Animator>();
        if (animator != null)
        {
            Destroy(animator);
        }
        enabled = false;
    }

    void OnDamaged()
    {
        EntityAudioManager.Instance.Play("Damaged" + Random.Range(1, 7), gameObject);
    }

    void OnSpottebByPlayer()
    {
        visibleToPlayer.visibilityRange = 6.6f;
    }

    void OnSightLostByPlayer()
    {
        visibleToPlayer.visibilityRange = 6.4f;
    }

    private bool IsAttacking()
    {
        return rangedWeaponAttackAbility != null && rangedWeaponAttackAbility.IsAttacking() || meleeWeaponAttackAbility != null && meleeWeaponAttackAbility.IsAttacking();
    }

    void Awake()
    {
        patrolPoints.Add(transform.position);
        patrolPointsEnumerator = patrolPoints.GetEnumerator();
        rb = GetComponent<Rigidbody2D>();
        currentMoveTarget = rb.position;
        oldPosition = rb.position;
        if (patrolPointsEnumerator.MoveNext())
        {
            currentMoveTarget = patrolPointsEnumerator.Current;
        }
    }
    private void Update()
    {
        if (rangedWeaponAttackAbility != null && rangedWeaponAttackAbility.IsAttacking())
        {
            if (transform.position.x - mainTarget.position.x < -distanceEpsilon)
            {
                transform.localScale = Vector3.one;
            }
            else if (transform.position.x - mainTarget.position.x > distanceEpsilon)
            {
                transform.localScale = inverseLocalScale;
            }
            oldPosition = rb.position;
            walking = false;
            return;
        }
        if (!walking && oldPosition != rb.position)
        {
            walking = true;
            //EntityAudioManager.Instance.PlayLoop("Footsteps", gameObject);
        }
        else if (walking && oldPosition == rb.position)
        {
            walking = false;
            //EntityAudioManager.Instance.StopLoop("Footsteps");
        }

        if (mainTarget.gameObject.activeInHierarchy)
        {
            if (mainTarget && rangedWeaponAttackAbility != null && !rangedWeaponAttackAbility.IsAttacking() && Vector2.Distance((Vector2)transform.position, (Vector2)mainTarget.transform.position) < visionDistance)
            {
                followingMainTarget = true;
                TryAttack();
            }
            else if (followingMainTarget && Vector2.Distance((Vector2)transform.position, (Vector2)mainTarget.transform.position) > visionDistance + .2f)
            {
                if (IsAttacking())
                {
                    rangedWeaponAttackAbility.StopAttack();
                }
                followingMainTarget = false;
            }
        }

        oldPosition = transform.position;
    }

    private void TryAttack()
    {
        if (mainTarget == null)
        {
            return;
        }
        if (rangedWeaponAttackAbility.CanAttack(mainTarget.GetComponent<CharacterHealth>()))
        {
            rangedWeaponAttackAbility.StartAttack(mainTarget.GetComponent<CharacterHealth>());
        }
        else
        {
            List<Collider2D> colliders = new();
            GetComponent<CircleCollider2D>().Overlap(new ContactFilter2D().NoFilter(), colliders);
            foreach (Collider2D collision in colliders)
            {
                StartAttack(collision);
            }
        }
    }
 
    private void StartAttack(Collider2D collision)
    {
        if (mainTarget == null)
        {
            return;
        }

        if (meleeWeaponAttackAbility.CanAttack() && !IsAttacking() && ReferenceEquals(collision.gameObject, mainTarget.gameObject))
        {
            rb.linearVelocity = Vector2.zero;
            meleeWeaponAttackAbility?.StartAttack(mainTarget.GetComponent<CharacterHealth>());
            //EntityAudioManager.Instance.Play("Scream" + Random.Range(1, 9));
        }
        else if (rangedWeaponAttackAbility.CanAttack() && !IsAttacking() && ReferenceEquals(collision.gameObject, mainTarget.gameObject))
        {
            rb.linearVelocity = Vector2.zero;
            rangedWeaponAttackAbility?.StartAttack(mainTarget.GetComponent<CharacterHealth>());
            //EntityAudioManager.Instance.Play("Scream" + Random.Range(1, 9));
        }
    }

    private void OnTriggerEnter2D(Collider2D collision)
    {
        StartAttack(collision);
    }

    private void OnTriggerExit2D(Collider2D collision)
    {
        if (meleeWeaponAttackAbility.CanAttack() && IsAttacking() && ReferenceEquals(collision.gameObject, mainTarget.gameObject))
        {
            meleeWeaponAttackAbility?.StopAttack();
        }
    }

    void FixedUpdate()
    {
        if (rangedWeaponAttackAbility != null && rangedWeaponAttackAbility.IsAttacking() || meleeWeaponAttackAbility != null && meleeWeaponAttackAbility.IsAttacking())
        {
            return;
        }

        if (followingMainTarget)
        {
            if (Vector2.Distance(rb.position, mainTarget.position) > distanceEpsilon)
            {
                rb.MovePosition(Vector2.MoveTowards(rb.position, mainTarget.position, speed * Time.deltaTime));
                if (transform.position.x - mainTarget.position.x < -distanceEpsilon)
                {
                    transform.localScale = Vector3.one;
                }
                else if (transform.position.x - mainTarget.position.x > distanceEpsilon)
                {
                    transform.localScale = inverseLocalScale;
                }
            }
        }
        else
        {
            if (Vector2.Distance(rb.position, currentMoveTarget) > distanceEpsilon)
            {
                rb.MovePosition(Vector2.MoveTowards(rb.position, currentMoveTarget, speed * Time.deltaTime));
                if (transform.position.x - currentMoveTarget.x < -distanceEpsilon)
                {
                    transform.localScale = Vector3.one;
                }
                else if (transform.position.x - currentMoveTarget.x > distanceEpsilon)
                {
                    transform.localScale = inverseLocalScale;
                }
            }
            else
            {
                if (!patrolPointsEnumerator.MoveNext())
                {
                    patrolPointsEnumerator = patrolPoints.GetEnumerator();
                    patrolPointsEnumerator.MoveNext();
                }
                currentMoveTarget = patrolPointsEnumerator.Current;
            }
        }
    }

    private void OnDestroy()
    {
        if (visibleToPlayer != null)
        {
            visibleToPlayer.OnSpottebByPlayer -= OnSpottebByPlayer;
            visibleToPlayer.OnSightLostByPlayer -= OnSightLostByPlayer;
        }
        if (characterHealth != null)
        {
            characterHealth.OnDied -= OnDied;
            characterHealth.OnDamaged -= OnDamaged;
        }
    }
}
