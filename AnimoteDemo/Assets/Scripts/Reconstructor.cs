using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class Reconstructor : MonoBehaviour
{
    public ConnectedUI ui;
    public AnimoteClient animote;

    private Reconstruction recon;

    private bool playing = false;
    private Transform target;

    // Working variables
    private long xyFrame = 0;
    private long disFrame = 0;
    private long oriFrame = 0;

    private bool smoothXY, smoothOri;

    private double workingTime = 0;
    private bool firstFrame = true;

    // Start is called before the first frame update
    void Start()
    {
        ui = FindObjectOfType<ConnectedUI>();

    }

    // Update is called once per frame
    void Update()
    {
        if (playing)
        {
            if (!firstFrame)
                workingTime += Time.smoothDeltaTime;  // Maybe Smooth Delta Time?

            bool xyDone = HandleXY();
            bool disDone = HandleDis();
            bool oriDone = HandleOri();

            if (xyDone && disDone && oriDone)
            {
                Stop();
                ui.SetStatus(ConnectedUI.Status.Idle);
            }else if (firstFrame)
            {
                firstFrame = false;
            }
        }
    }

    private bool HandleXY()
    {
        if (recon.GetXYFrames().Count == 0)
            return true;

        XYFrame frame = (XYFrame)recon.GetXYFrames()[(int)xyFrame];

        if (xyFrame >= recon.GetXYFrames().Count - 1)    // last frame
        {
            return true;
        } 
        else if (firstFrame)
        {
            SnapToXYFrame(frame);
            return false;
        }

        XYFrame nextFrame = (XYFrame)recon.GetXYFrames()[(int)xyFrame + 1];

        if (workingTime >= nextFrame.time)  // time for next frame
        {
            SnapToXYFrame(nextFrame);
            xyFrame++;
        }
        else if(smoothXY)
        {                                   // interpolate inbetween frames

            float ratio = (float) ((workingTime - frame.time) / (nextFrame.time - frame.time));
            SnapToXYFrame(new XYFrame { xy = Vector2.Lerp(frame.xy, nextFrame.xy, ratio), time = 0 });
        }

        return false;
    }

    private bool HandleDis()
    {
        if (recon.GetDisFrames().Count == 0)
            return true;

        DisFrame frame = (DisFrame)recon.GetDisFrames()[(int)disFrame];

        if (disFrame >= recon.GetDisFrames().Count - 1)    // last frame
        {
            return true;
        }
        else if (firstFrame)
        {
            SnapToDisFrame(frame);
            return false;
        }

        DisFrame nextFrame = (DisFrame)recon.GetDisFrames()[(int)disFrame + 1];

        if (workingTime >= nextFrame.time)  // time for next frame
        {
            SnapToDisFrame(nextFrame);
            disFrame++;
        }
        else if (smoothXY)
        {                                   // interpolate inbetween frames

            float ratio = (float)((workingTime - frame.time) / (nextFrame.time - frame.time));
            SnapToDisFrame(new DisFrame { distance = Mathf.Lerp(frame.distance, nextFrame.distance, ratio), time = 0 });
        }

        return false;
    }

    private bool HandleOri()
    {
        if (recon.GetOriFrames().Count == 0)
            return true;

        OriFrame frame = (OriFrame)recon.GetOriFrames()[(int)oriFrame];

        if (oriFrame >= recon.GetOriFrames().Count - 1)    // last frame
        {
            return true;
        }
        else if (firstFrame)
        {
            SnapToOriFrame(frame);
            return false;
        }

        OriFrame nextFrame = (OriFrame)recon.GetOriFrames()[(int)oriFrame + 1];

        if (workingTime >= nextFrame.time)  // time for next frame
        {
            SnapToOriFrame(nextFrame);
            oriFrame++;
        }
        else if (smoothOri)
        {                                   // interpolate inbetween frames

            float ratio = (float)((workingTime - frame.time) / (nextFrame.time - frame.time));
            SnapToOriFrame(new OriFrame { orientation = Vector3.Lerp(frame.orientation, nextFrame.orientation, ratio), time = 0 });
        }

        return false;
    }

    private void SnapToXYFrame(XYFrame frame)
    {
        target.position = new Vector3(frame.xy.x, frame.xy.y, target.position.z);
    }

    private void SnapToDisFrame(DisFrame frame)
    {
        target.position = new Vector3(target.position.x, target.position.y, frame.distance);
    }

    private void SnapToOriFrame(OriFrame frame)
    {
        //Vector3 currentRot = target.rotation.eulerAngles;
        //Vector3 rotation = frame.orientation - currentRot;

        //target.Rotate(rotation, Space.Self);

        Quaternion q = Quaternion.Euler(frame.orientation.x, frame.orientation.y, frame.orientation.z);
        target.SetPositionAndRotation(target.position, q);
    }

    public bool Export(Reconstruction reconstruction, Transform t)
    {
        bool goodExport = Play(reconstruction, t);
        ui.SetStatus(ConnectedUI.Status.Export);
        animote.StartExport();
        return goodExport;
    }

    public bool Play(Reconstruction reconstruction, Transform t)
    {
        recon = reconstruction;

        ui.SetStatus(ConnectedUI.Status.Replay);

        xyFrame = 0;
        disFrame = 0;
        oriFrame = 0;

        smoothXY = ui.settings.SmoothXY();
        smoothOri = ui.settings.SmoothOri();

        workingTime = 0;

        target = t;
        firstFrame = true;
        playing = true;

        return true;
    }

    public void Stop()
    {
        if(animote.exporting)
            animote.StopExport();

        playing = false;

        target = null;
    }

    public Reconstruction Reconstruct(AnimoteAnimation animation)
    {

        Debug.Log("Reconstructing...");

        int FPS = animation.GetFPS();

        ArrayList keyFrames = animation.GetFrames();
        Reconstruction recon = new Reconstruction(FPS);

        XYFrame xy;
        DisFrame dis;
        OriFrame ori;
        double startTime = ((KeyFrame)keyFrames[0]).time;
        double time;

        foreach(KeyFrame frame in keyFrames)
        {
            xy = new XYFrame();
            dis = new DisFrame();
            ori = new OriFrame();

            // XY
            xy.xy = AnimoteClient.DecodePixyProjection(frame.x, frame.y);

            // Distance
            dis.distance = AnimoteClient.DecodePixyDistance(frame.w, frame.h);

            // Orientation
            ori.orientation = AnimoteClient.MapOrientation(frame.heading, frame.pitch, frame.roll, frame.acc);

            // Actual time taken on Server
            time = (double)(((double)frame.time - startTime) / 1000f);
            
            // Add frames if valid
            if (xy.xy != Vector2.zero)
            {
                xy.time = time;
                dis.time = time;

                recon.AddXYFrame(xy);
                recon.AddDisFrame(dis);
            }
            if (ori.orientation != Vector3.zero)
            {
                ori.time = time;
                recon.AddOriFrame(ori);
            }

        }

        // Reconstruct distance frames
        recon.ReconstructDistanceFrames();

        Debug.Log("Reconstruction finished: #xyframes = " + recon.GetXYFrames().Count + " #disframes = " + recon.GetDisFrames().Count + " #oriframes = " + recon.GetOriFrames().Count);

        return recon;
    }
}

public struct XYFrame
{
    public Vector2 xy;
    public double time;
}

public struct DisFrame
{
    public float distance;
    public double time;
}

public struct OriFrame
{
    public Vector3 orientation;
    public double time;
}

public class Reconstruction
{
    private int FPS;
    private ArrayList xyFrames, disFrames, oriFrames;

    public Reconstruction(int FPS)
    {
        this.FPS = FPS;
        xyFrames = new ArrayList();
        disFrames = new ArrayList();
        oriFrames = new ArrayList();
    }

    public int GetFPS()
    {
        return FPS;
    }

    public ArrayList GetXYFrames()
    {
        return xyFrames;
    }

    public ArrayList GetDisFrames()
    {
        return disFrames;
    }
    public ArrayList GetOriFrames()
    {
        return oriFrames;
    }

    public void AddXYFrame(XYFrame frame)
    {
        xyFrames.Add(frame);
    }

    public void AddDisFrame(DisFrame frame)
    {
        disFrames.Add(frame);
    }

    public void AddOriFrame(OriFrame frame)
    {
        oriFrames.Add(frame);
    }

    public void ReconstructDistanceFrames()
    {
        // curve fitting
    }
}