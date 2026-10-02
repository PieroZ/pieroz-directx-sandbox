using UnityEngine;
using UnityEngine.UI;

public class LevelMap : MonoBehaviour
{
    public int diameter = 256;

    private int radius = 128;
    private Texture2D texture;
    private RawImage rawImage;
    private Camera mainCamera;
    private Vector3Int lastPosition = Vector3Int.zero;
    private int radiusSquared;

    private void Awake()
    {
        radius = diameter / 2;
        radiusSquared = radius * radius;
    }

    private void Start()
    {
        mainCamera = Camera.main;
        rawImage = GetComponent<RawImage>();
        texture = new Texture2D(diameter, diameter);
        for (int x = 0; x < texture.width; x++)
        {
            for (int y = 0; y < texture.height; y++)
            {
                texture.SetPixel(x, y, Color.clear);
            }
        }
        texture.Apply();
        rawImage.texture = texture;
    }
    private void Update()
    {
        GeneratePerlinNoiseImage();
    }

    private void GeneratePerlinNoiseImage()
    {
        Vector3Int currentPosition = WorldMapManager.Instance.WorldToTilemapCoord(mainCamera.transform.position);
        if (currentPosition == lastPosition)
        {
            return;
        }
        //Debug.Log(currentPosition);

        for (int x = 0; x < texture.width; x++)
        {
            for (int y = 0; y < texture.height; y++)
            {
                if ((x - radius) * (x - radius) + (y - radius) * (y - radius) > radiusSquared)
                {
                    texture.SetPixel(x, y, Color.clear);
                    continue;
                }
                float perlinValue = WorldMapManager.Instance.GetPerlinValueForTile(x + currentPosition.x - radius, y + currentPosition.y - radius);
                if (perlinValue < .5f)
                {
                    perlinValue = 1f;
                }
                else if (perlinValue < .75f)
                {
                    perlinValue = .5f;
                }
                else if (perlinValue < .875f)
                {
                    perlinValue = .25f;
                }
                else
                {
                    perlinValue = 0f;
                }
                Color color = new(perlinValue, perlinValue * .8f, perlinValue *.6f);
                texture.SetPixel(x, y, color);
            }
        }

        if (WorldMapManager.Instance.IsTargetGenerated())
        {
            DrawMinimapMarker(currentPosition, WorldMapManager.Instance.GetTargetLocation(), Color.red);
        }

        if (WorldMapManager.Instance.IsExitGenerated())
        {
            DrawMinimapMarker(currentPosition, WorldMapManager.Instance.GetExitLocation(), Color.yellow);
        }

        if (WorldMapManager.Instance.IsEntranceGenerated())
        {
            DrawMinimapMarker(currentPosition, WorldMapManager.Instance.GetEntranceLocation(), Color.blue);
        }

        for (int x = -3; x <= 3; x++)
        {
            for (int y = -3; y <= 3; y++)
            {
                if (Mathf.Pow(x, 2) + Mathf.Pow(y, 2) < 4)
                {
                    texture.SetPixel(radius + x, radius + y, Color.green);
                }
            }
        }

        texture.SetPixel(radius, radius, Color.red);
        texture.Apply();
        rawImage.texture = texture;
        lastPosition = currentPosition;
    }

    private void DrawMinimapMarker(Vector3Int currentPosition, Vector3Int targetLocation, Color color)
    {
        Vector3 targetDirection = targetLocation - currentPosition;
        if (targetDirection.magnitude > radius)
        {
            targetDirection.Normalize();
            Vector3 projectionPoint = targetDirection * radius;
            targetLocation.x = Mathf.FloorToInt(projectionPoint.x);
            targetLocation.y = Mathf.FloorToInt(projectionPoint.y);
        }
        else
        {
            targetLocation.x = Mathf.FloorToInt(targetDirection.x);
            targetLocation.y = Mathf.FloorToInt(targetDirection.y);
        }
        int circleRadius = 4;
        int circleRadiusSquared = circleRadius * circleRadius;
        for (int x = -circleRadius + 1; x <= circleRadius; x++)
        {
            for (int y = -circleRadius + 1; y <= circleRadius; y++)
            {
                if (x * x + y * y > circleRadiusSquared)
                {
                    continue;
                }
                if (targetLocation.x + x > -radius
                    && targetLocation.x + x < radius
                    && targetLocation.y + y > -radius
                    && targetLocation.y + y < radius)
                {
                    texture.SetPixel(targetLocation.x - radius + x, targetLocation.y - radius + y, color);
                }
            }
        }
    }
}
