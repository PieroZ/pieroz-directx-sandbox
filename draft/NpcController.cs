using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class NpcController : MonoBehaviour
{
    public float speed = 1f;
    public GameObject patrolPointsList;
    private List<Vector3> patrolPoints = new();
    public Transform mainTarget = null;
    public float visionDistance = 6.4f;
    private List<Vector3>.Enumerator patrolPointsEnumerator;
    private const float distanceEpsilon = 0.01f;
    private const float maximalDistanceFromTarget = 0.64f;
    private Rigidbody2D rb;
    private Vector2 currentMoveTarget;
    private bool followingMainTarget;
    private VisibleToPlayer visibleToPlayer = null;
    private Vector2 oldPosition = Vector2.zero;
    private bool walking = false;
    private bool fixedUpdateWalking = false;
    [SerializeField] public string monsterTypeName = "DefaultNpcName";
    private Vector3 inverseLocalScale = new(-1, 1, 1);
    private Animator anim;

    IEnumerator VoiceGenerator()
    {
        while (true)
        {
            yield return new WaitForSeconds(Random.Range(5f,30f));
            //EntityAudioManager.Instance.Play("Growl" + Random.Range(1, 8), gameObject);
        }
    }

    private void OnMouseDown()
    {
        //EntityAudioManager.Instance.Play("Growl" + Random.Range(1, 8), gameObject);
    }

    private void Start()
    {
        anim = GetComponentInChildren<Animator>();
        if (mainTarget == null)
        {
            PlayerController playerController = PlayerManager.Instance.GetPlayerController();
            if (playerController != null)
            {
                mainTarget = playerController.transform;
            }
        }
        if (TryGetComponent(out visibleToPlayer))
        {
            visibleToPlayer.OnSpottebByPlayer += OnSpottebByPlayer;
            visibleToPlayer.OnSightLostByPlayer += OnSightLostByPlayer;
        }
        StartCoroutine(VoiceGenerator());
    }

    void OnSpottebByPlayer()
    {
        visibleToPlayer.visibilityRange = 6.6f;
    }

    void OnSightLostByPlayer()
    {
        visibleToPlayer.visibilityRange = 6.4f;
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
        if (!walking && fixedUpdateWalking)
        {
            walking = true;
            anim.SetTrigger("TrWalk");
        }
        else if (walking && !fixedUpdateWalking)
        {
            walking = false;
            anim.SetTrigger("TrIdle");
        }
        if (mainTarget == null)
        {
            return;
        }
        if (mainTarget && !followingMainTarget && Vector2.Distance(transform.position, mainTarget.transform.position) < visionDistance)
        {
            followingMainTarget = true;
        }
        else if (followingMainTarget && Vector2.Distance(transform.position, mainTarget.transform.position) > visionDistance + .2f)
        {
            followingMainTarget = false;
        }
    }

    void FixedUpdate()
    {
        if (mainTarget != null && followingMainTarget)
        {
            if (Vector2.Distance(rb.position, mainTarget.position) > maximalDistanceFromTarget)
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
            else
            {
                rb.linearVelocity = Vector2.zero;
            }
        }

        fixedUpdateWalking = Vector2.Distance(oldPosition, rb.position) > distanceEpsilon;
        oldPosition = rb.position;
    }

    private void OnDestroy()
    {
        if (visibleToPlayer != null)
        {
            visibleToPlayer.OnSpottebByPlayer -= OnSpottebByPlayer;
            visibleToPlayer.OnSightLostByPlayer -= OnSightLostByPlayer;
        }
    }
}
