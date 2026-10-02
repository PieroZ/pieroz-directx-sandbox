using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class AutoDestroyAfterAnimation : MonoBehaviour
{
    void Start()
    {
        float time = 0f;
        if (TryGetComponent(out Animator animator))
        {
            AnimationClip[] clips = animator.runtimeAnimatorController.animationClips;
            foreach (AnimationClip clip in clips)
            {
                time = Mathf.Max(time, clip.length);
            }
        }
        if (TryGetComponent(out AudioSource audioSource))
        {
            time = Mathf.Max(time, audioSource.clip.length);
        }
        Destroy(gameObject, time);
    }
}
