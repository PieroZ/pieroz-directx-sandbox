using System.Collections;
using UnityEngine;

public class StoryEventHead : StoryEventStep
{
    public string storyName = "defaultStoryName";

    public void TriggerNextStoryEventStep()
    {
        if (nextStoryEventStep != null)
        {
            StoryManager.Instance.TriggerNextStoryEventStep(this);
        }
    }

    private void Start()
    {
        TriggerNextStoryEventStep();
    }

    private void Update()
    {
        enabled = false;
    }
}
