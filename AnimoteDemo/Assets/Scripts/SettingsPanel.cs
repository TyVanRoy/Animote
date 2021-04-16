using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

public class SettingsPanel : MonoBehaviour
{
    public int DefaultRes = 1;
    public string DefaultTitle = "Animote Animation";
    [SerializeField] private static int MAX_FPS = 100;
    [SerializeField] private Text animTitle, posRes, oriRes;
    [SerializeField] private Toggle doPos, doOri, smoothXY, smoothOri;
    [SerializeField] private Toggle enHeading, enPitch, enRoll, enX, enY, enZ;

    private const uint XAXIS = 0b001;
    private const uint YAXIS = 0b010;
    private const uint ZAXIS = 0b100;

    private const uint HEADING = 0b001;
    private const uint PITCH = 0b010;
    private const uint ROLL = 0b100;

    public string GetAnimTitle()
    {
        if (animTitle.text != "")
            return animTitle.text;
        else
            return DefaultTitle;
    } 

    public void TogglePositionEnable()
    {
        enX.interactable = doPos.isOn;
        enY.interactable = doPos.isOn;
        enZ.interactable = doPos.isOn;
    }

    public void ToggleOrientationEnable()
    {
        enHeading.interactable = doOri.isOn;
        enPitch.interactable = doOri.isOn;
        enRoll.interactable = doOri.isOn;
    }

    public int GetPositionResolution()
    {
        if (posRes.text != "")
            return int.Parse(posRes.text) > MAX_FPS ? MAX_FPS : int.Parse(posRes.text);
        else
            return (DefaultRes);
    }

    public int GetOrientationResolution()
    {
        if (oriRes.text != "")
            return int.Parse(oriRes.text) > MAX_FPS ? MAX_FPS : int.Parse(oriRes.text);
        else
            return (DefaultRes);
    }

    public bool DoPosition()
    {
        return doPos.isOn;
    }

    public bool DoOrientation()
    {
        return doOri.isOn;
    }

    public bool SmoothXY()
    {
        return smoothXY.isOn;
    }

    public bool SmoothOri()
    {
        return smoothOri.isOn;
    }

    public uint GetPosAxis()
    {
        return (uint) ((enX.isOn ? XAXIS : 0) | (enY.isOn ? YAXIS : 0) | (enZ.isOn ? ZAXIS : 0));
    }

    public uint GetOriAxis()
    {
        return (uint) ((enHeading.isOn ? HEADING : 0) | (enPitch.isOn ? PITCH : 0) | (enRoll.isOn ? ROLL : 0));
    }
}
