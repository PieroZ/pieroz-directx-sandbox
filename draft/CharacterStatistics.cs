using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class CharacterStatistics : MonoBehaviour
{
    public EnumIntArray basePrimaryStatistics = new(typeof(PrimaryStatistics));
    private EnumIntArray totalPrimaryStatistics = new(typeof(PrimaryStatistics));

    public delegate void OnStatisticsChangedHandler();
    public OnStatisticsChangedHandler OnStatisticsChanged;

    public int GetBasePrimaryStatistic(PrimaryStatistics statistic)
    {
        return basePrimaryStatistics.Get(statistic);
    }

    public void SetBasePrimaryStatistic(PrimaryStatistics statistic, int value)
    {
        basePrimaryStatistics.Set(statistic, value);
        OnStatisticsChanged?.Invoke();
    }
    public int GetTotalPrimaryStatistic(PrimaryStatistics statistic)
    {
        return totalPrimaryStatistics.Get(statistic);
    }

    public void SetTotalPrimaryStatistic(PrimaryStatistics statistic, int value)
    {
        totalPrimaryStatistics.Set(statistic, value);
        OnStatisticsChanged?.Invoke();
    }
}
