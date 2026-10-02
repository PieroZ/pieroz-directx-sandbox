using UnityEngine;
using System.Linq;

public class RandomArtifactItemGenerator : MonoBehaviour
{
    public ScriptableItem[] artifactTemplates;
    private void Start()
    {
        if (TryGetComponent(out ConstrainedInventory constrainedInventory))
        {
            ScriptableItem artifact = Instantiate(artifactTemplates[Random.Range(0, artifactTemplates.Length)]);
            foreach (var additionalAttribute in artifact.additionalAttributes.ToList())
            {
                artifact.additionalAttributes[additionalAttribute.Key] = Random.Range(0, artifact.additionalAttributes[additionalAttribute.Key]);
            }

            switch (Random.Range(0, 2))
            {
                case 0:
                    artifact.additionalAttributes.Add(ItemAdditionalAttributes.UseInPlace, 1);
                    break;
                case 1:
                    artifact.additionalAttributes.Add(ItemAdditionalAttributes.UseShotAtTarget, 1);
                    break;
            }
            artifact.additionalAttributes.Add(ItemAdditionalAttributes.AreaOfEffectRadius, Random.Range(1, 6));

            switch (artifact.itemName)
            {
                case "Book Of The Dead":
                {
                    artifact.additionalAttributes.Add(ItemAdditionalAttributes.OnUseBurn, Random.Range(1, 4));
                    break;
                }
                case "Eye of the Nile":
                {
                    artifact.additionalAttributes.Add(ItemAdditionalAttributes.OnUsePoison, Random.Range(1, 4));
                    break;
                }
                case "Dry Ice":
                {
                    artifact.additionalAttributes.Add(ItemAdditionalAttributes.OnUseFreeze, Random.Range(1, 4));
                    break;
                }
                case "Ring of Cold":
                {
                    artifact.additionalAttributes.Add(ItemAdditionalAttributes.OnUseElectrocut, Random.Range(1, 4));
                    break;
                }
            }

            artifact.additionalAttributes.Add(ItemAdditionalAttributes.OnUseDamage, Random.Range(5, 14));
            artifact.additionalAttributes.Add(ItemAdditionalAttributes.OnUseCooldown, Random.Range(1, 4));

            artifact.value = 1f;
            constrainedInventory.Add(new(artifact));
        }
    }
}
