using System;
using System.Collections;
using System.Collections.Generic;
using System.Net;
using System.Net.Sockets;
using System.Text;
using UnityEngine;
using UnityEngine.Networking;
// using UnityEngine.UIElements;

public class AnimoteClient : MonoBehaviour
{
    public static AnimoteClient instance;

    private const uint XAXIS = 0b001;
    private const uint YAXIS = 0b010;
    private const uint ZAXIS = 0b100;

    private const uint HEADING = 0b001;
    private const uint PITCH = 0b010;
    private const uint ROLL = 0b100;


    // PIXY
    private const int PIXY_X_MAX = 315;
    private const int PIXY_Y_MAX = 205;

    public bool NO_NET = true;
    public bool DO_LOG = true;

    public bool HeadingEnable = false;
    public bool PitchEnable = false;
    public bool RollEnable = false;

    public bool POS_TEST = true;
    public bool POS_STOP = false;
    [Range(0, PIXY_X_MAX)]
    public uint pixyTestX;
    [Range(0, PIXY_Y_MAX)]
    public uint pixyTestY;
    [Range(0, 10)]
    public uint testDistance;

    public int calibrationCountTotal = 40;
    private int calCount = 0;

    public bool exporting = false;

    private double testTime;

    public Reconstructor reconstructor;
    public Material visibleMaterial, obscuredMaterial;

    public bool doPosition = false;
    public bool doOrientation = true;
    public bool smoothPositionOnLapse = true;
    public bool IR_0 = true;
    public bool IR_1 = true;
    public bool IR_2 = true;
    public static int MAX_BLOB_SIZE = 42;
    public static int MIN_BLOB_SIZE = 10;
    public static int xyzBound = 10;
    public int historyLength = 50;
    public double dataQTime = .3;
    public int IMUSendWait = 100;
    public double readyQTime = 1;
    public double goQTime = .3;

    public ConnectedUI ui;
    public UnityAnimationRecorder recorder;
    public Countdown counter;

    // UDP
    private static bool connected = true;
    private static int port = 3333;

    IPEndPoint remoteEndPoint;
    UdpClient client;

    /*
     * COM CONSTANTS
     */
    private const string IP = "192.168.1.1";
    private const string V_START = "/data_start:";
    private const string PREFIX = "/Unity";

    /*
     * COM QUESTIOS
     */
    private const string CURRENT_Q = "/current?";
    private const string READY_Q = "/ready?";
    private const string IMU_RATE_Q = "/IMU_rate";
    private const string GO_Q = "/go?";

    /*
     * COM RESPONSES
     */
    private const string READY_R = "ready_confirm";
    private const string GO_R = "go_confirm";
    private const string STOP_S = "/stop";

    private Queue<string> currentQ;

    private Queue<Vector3> posHistory;

    private double qClock = 0;
    private bool readyQ = false;
    private bool goQ = false;
    private bool firstFrame = false;
    private bool visible = true;

    private UInt16 irSelect = 0;

    private Renderer renderer;

    // ANIMATION

    private KeyFrame previewTarget;
    private AnimoteAnimation workingAnimation;
    private static float headingBase = 0;

    private bool calibrating = false;

    public enum OperatingMode {
        Idle,
        WaitingForButtonRelease,
        Animating,
        Calibrating
    };

    OperatingMode opMode;

    private void Awake()
    {
        instance = this;
    }

    void Start()
    {
        ui = FindObjectOfType<ConnectedUI>();
        reconstructor = FindObjectOfType<Reconstructor>();
        renderer = GetComponent<Renderer>();

        if (!NO_NET)
            UDP_Init();

        currentQ = new Queue<string>();
        posHistory = new Queue<Vector3>(historyLength);
        opMode = OperatingMode.Idle;
        ui.SetStatus(ConnectedUI.Status.Idle);
    }

    private void Update()
    {
        if (!connected)
        {
            return;
        }

        switch (opMode)
        {
            case OperatingMode.Idle:

                if (POS_TEST && !POS_STOP)
                {
                    BeginAnimating();
                    testTime = 0;
                }

                if (qClock >= readyQTime)
                {
                    qClock = 0;
                    AskHostFor(READY_Q);
                }
                if (readyQ)
                {
                    dataQTime = calibrating ? 0.1f : (1f / ((double)ui.settings.GetPositionResolution()));
                    IMUSendWait = calibrating ? 40 : ((int) (500 / (((double)ui.settings.GetOrientationResolution()))));

                    Debug.Log("DQTIME = " + dataQTime);

                    GiveHostIMURate();
                    qClock = 0;
                    opMode = OperatingMode.WaitingForButtonRelease;

                    if (calibrating)
                    {
                        ui.SetStatus(ConnectedUI.Status.HCal_working);
                    }
                    else
                    {
                        ui.SetStatus(ConnectedUI.Status.ReadySet);
                    }

                    break;
                }

                qClock += Time.deltaTime;

                break;
            case OperatingMode.WaitingForButtonRelease:

                if (qClock >= goQTime)
                {
                    qClock = 0;
                    AskHostFor(GO_Q);
                }
                if (goQ)
                {
                    qClock = 0;
                    if (calibrating)
                    {
                        BeginCalibrating();
                    }
                    else
                    {
                        BeginAnimating();
                    }
                    break;
                }

                qClock += Time.deltaTime;

                break;
            case OperatingMode.Animating:

                if (POS_TEST)
                {
                    if (POS_STOP)
                        StopAnimating();

                    firstFrame = true;
                    previewTarget = new KeyFrame
                    {
                        isValid = true,
                        x = pixyTestX,
                        y = pixyTestY,
                        w = 10,
                        h = 10,
                        heading = 0,
                        pitch = 0,
                        roll = 0,
                        time = (ulong)(testTime * 1000f)
                    };
                    testTime += Time.deltaTime;
                    workingAnimation.AddFrame(previewTarget);
                }
                else if (currentQ.Count > 0)
                {
                    if (!POS_TEST)
                    {
                        previewTarget = ParseDataPacket(currentQ.Dequeue());

                        // Position Axis Enable
                        uint axis = ui.settings.GetPosAxis();

                        previewTarget.x = (!doPosition || !((axis & XAXIS) == 0 ? false : true)) ? (PIXY_X_MAX / 2) : previewTarget.x;

                        previewTarget.y = (!doPosition || !((axis & YAXIS) == 0 ? false : true)) ? (PIXY_Y_MAX / 2) : previewTarget.y;

                        previewTarget.w = (!doPosition || !((axis & ZAXIS) == 0 ? false : true)) ? ((uint)(MAX_BLOB_SIZE - MIN_BLOB_SIZE) / 2) : previewTarget.w;
                        previewTarget.h = (!doPosition || !((axis & ZAXIS) == 0 ? false : true)) ? ((uint)(MAX_BLOB_SIZE - MIN_BLOB_SIZE) / 2) : previewTarget.h;

                        // Orientation Axis Enable
                        axis = ui.settings.GetOriAxis();

                        previewTarget.heading = (!doOrientation || !((axis & HEADING) == 0 ? false : true)) ? 0 : previewTarget.heading;

                        // Switched for convinience
                        previewTarget.pitch = (!doOrientation || !((axis & ROLL) == 0 ? false : true)) ? 0 : previewTarget.pitch;
                        previewTarget.roll = (!doOrientation || !((axis & PITCH) == 0 ? false : true)) ? 0 : previewTarget.roll;

                        if (counter.Done())
                        {
                            workingAnimation.AddFrame(previewTarget);
                        }

                        firstFrame = true;
                    }
                    if (DO_LOG)
                        Debug.Log("PIXY => x: " + previewTarget.x + " y: " + previewTarget.y + " w: " + previewTarget.w + " h: " + previewTarget.h + "\nIR Voltages => v0: " + previewTarget.irv0 + " v1: " + previewTarget.irv1 + "\nIMU => Heading: " + previewTarget.heading + " Pitch: " + previewTarget.pitch + " Roll: " + previewTarget.roll);
                }
                if (qClock >= dataQTime)
                {
                    irSelect = EncodeIRSelect();
                    AskHostFor(CURRENT_Q + irSelect);
                    qClock -= dataQTime;
                }
                qClock += Time.deltaTime;

                if (previewTarget.isValid && firstFrame)
                {
                    UpdateLivePositionPreview();
                    UpdateLiveOrientationPreview();
                }

                break;

            case OperatingMode.Calibrating:

                if(calCount >= calibrationCountTotal)
                {
                    ui.SetStatus(ConnectedUI.Status.HCal_ready);
                }
                else if (currentQ.Count > 0)
                {
                    previewTarget = ParseDataPacket(currentQ.Dequeue());
                    if (previewTarget.isValid && (previewTarget.pitch != 0 && previewTarget.heading != 0 && previewTarget.roll != 0))
                    {
                        workingAnimation.AddFrame(previewTarget);
                        calCount++;
                    }
                }
                if (qClock >= dataQTime)
                {
                    AskHostFor(CURRENT_Q + irSelect);
                    qClock -= dataQTime;
                }
                qClock += Time.deltaTime;

                break;
        }

    }

    private UInt16 EncodeIRSelect()
    {
        return (UInt16) ((IR_2 ? 0b100 : 0) & (IR_1 ? 0b010 : 0) & (IR_0 ? 0b001 : 0));
    }

    private void BeginAnimating()
    {
        currentQ.Clear();
        reconstructor.Stop();

        firstFrame = false;
        workingAnimation = new AnimoteAnimation(ui.settings.GetPositionResolution());

        doPosition = ui.settings.DoPosition();
        doOrientation = ui.settings.DoOrientation();

        opMode = OperatingMode.Animating;
        ui.SetStatus(ConnectedUI.Status.Animating);

        counter.Begin();

        Debug.Log("Beginning Animation. dataQTime = " + dataQTime + " IMUSendWait = " + IMUSendWait);
    }

    private void BeginCalibrating()
    {
        workingAnimation = new AnimoteAnimation(ui.settings.GetPositionResolution());
        calCount = 0;

        currentQ.Clear();
        opMode = OperatingMode.Calibrating;
    }

    private void StopAnimating()
    {
        opMode = OperatingMode.Idle;
        readyQ = false;
        goQ = false;
        Debug.Log("Animation stopped");

        ui.animations.AddAnimation(workingAnimation, ui.settings.GetAnimTitle());
        ui.SetStatus(ConnectedUI.Status.Idle);
    }

    private void StopCalibrating()
    {
        calibrating = false;

        // Calculate headingBase

        // Get headings
        ArrayList headings = new ArrayList();
        foreach(KeyFrame frame in workingAnimation.GetFrames())
        {
            headings.Add(frame.heading);
        }

        // Average the last 50% excluding the last 10% of values.
        float sum = 0;
        int lazyCount = 0;
        for(int i = headings.Count / 2; i < (headings.Count - (headings.Count / 10)); i++)
        {
            sum += (float) headings[i];
            lazyCount++;
        }
        headingBase = sum / lazyCount;

        opMode = OperatingMode.Idle;
        readyQ = false;
        goQ = false;
        Debug.Log("Calibration finished. headingBase =  " + headingBase);
        ui.SetStatus(ConnectedUI.Status.Idle);
    }

    public void StartExport()
    {
        exporting = true;
        recorder.fileName = ui.settings.GetAnimTitle();
        recorder.startRecord = true;
    }

    public void StopExport()
    {
        exporting = false;
        recorder.stopRecord = true;
    }

    public void CalibrateHeading()
    {
        calibrating = true;
        ui.SetStatus(ConnectedUI.Status.HCal_init);
    }

    private void UpdateLivePositionPreview()
    {
        if (!doPosition)
            return;

        Vector3 f = Vector3.zero;
        Vector2 xy = DecodePixyProjection(previewTarget.x, previewTarget.y);            // decode the pixy XY

        if (xy == Vector2.zero)
        {
            renderer.material = obscuredMaterial;
            visible = false;

            if (smoothPositionOnLapse)                                                  // handle lapse if it occurs
            {

                Vector3[] history = new Vector3[historyLength];
                posHistory.CopyTo(history, 0);

                Vector3 avgDel = Vector3.zero;
                for(int i = 0; i < historyLength - 1; i++)
                {
                    avgDel += (history[i] - history[i + 1]);
                }
                avgDel /= historyLength;

                f = this.transform.position + avgDel;
            }
        }
        else {

            if (!visible)
            {
                renderer.material = visibleMaterial;
                visible = true;
            }

            float distance = DecodePixyDistance(previewTarget.w, previewTarget.h);          // decode the pixy distance
            DecodeIRDistance(previewTarget.irv0, previewTarget.irv0);
            Vector3 xyz = GetXYZ(xy, distance);                                             // calculate XYZ

            f = xyz;   // move
        }

        if (smoothPositionOnLapse)
        {
            posHistory.Enqueue(this.transform.position);
        }

        this.transform.position = f;
    }

    public static Vector2 DecodePixyProjection(float targetX, float targetY)
    {
        if (targetX > PIXY_X_MAX || targetY > PIXY_Y_MAX)
            return Vector2.zero;

        Vector2 xy = new Vector2();

        xy.x = Mathf.Lerp(-xyzBound, xyzBound, 1 - (targetX / (float)PIXY_X_MAX));
        xy.y = Mathf.Lerp(-xyzBound, xyzBound, 1 - (targetY / (float)PIXY_Y_MAX));

        return xy;
    }

    public static float DecodePixyDistance(float targetW, float targetH)
    {
        float small = targetW > targetH ? targetH : targetW;
        float big = small == targetW ? targetH : targetW;

        float pos = (big - MIN_BLOB_SIZE) / (MAX_BLOB_SIZE - MIN_BLOB_SIZE);

//        Debug.Log("S = " + big + "\tpos = " + pos);

        return Mathf.Lerp(-xyzBound, xyzBound, pos);
    }

    public static float DecodeIRDistance(float raw1, float raw2)
    {
        float max = raw1 > raw2 ? raw1 : raw2;
        float pos = (max - 90) / 1024;
        return Mathf.Lerp(-xyzBound, +xyzBound, pos);
    }

    // Possibly add skewing to this??
    public Vector3 GetXYZ(Vector2 xy, float distance)
    {
        return new Vector3(xy.x, xy.y, distance);
    }

    private void UpdateLiveOrientationPreview()
    {
        if (!doOrientation || (counter.PercentComplete() < .5 && !counter.Done()))
            return;

        Vector3 targetRot = MapOrientation(previewTarget.heading, previewTarget.pitch, previewTarget.roll, previewTarget.acc);

        if (targetRot == Vector3.zero)
            return;

        //Vector3 currentRot = this.transform.rotation.eulerAngles;
        //Vector3 rotation = targetRot - currentRot;
        //this.transform.Rotate(new Vector3(0, rotation.y, 0), Space.World);

        Quaternion q = Quaternion.Euler(targetRot.x, targetRot.y, targetRot.z);
        this.transform.SetPositionAndRotation(this.transform.position, q);

        /*
        // 2
        Vector3 currentRot = this.transform.rotation.eulerAngles;
        Vector3 rotation = targetRot - currentRot;
        this.transform.Rotate(rotation, Space.Self);
        */
    }

    public static Vector3 MapOrientation(float heading, float pitch, float roll, Vector3 acc)
    {
        if (heading == 0 && pitch == 0 && roll == 0)
            return Vector3.zero;

        bool upsideDown = (acc.y > 0);

        roll += 90;

        float zt = pitch == 0 ? 0 : (upsideDown ? (pitch - 180) : -pitch);
        float yt = heading == 0 ? 0 : upsideDown ? (headingBase - heading + 180) : (headingBase - heading);
        float xt = roll == 90 ? 0 : (upsideDown ? -roll : -roll);

        if (Math.Abs(zt) > 60)
        {
            //xt = 0;
        }
        else if (Math.Abs(xt) > 60)
        {
            //zt = 0;
        }

        return new Vector3(xt, yt, zt);
    }

    private void AskHostFor(string QUESTION) {
        StartCoroutine(MakeRequest(PREFIX + QUESTION));
    }

    private void GiveHostIMURate()
    {
        AskHostFor(IMU_RATE_Q + IMUSendWait.ToString());
    }

    private KeyFrame ParseDataPacket(string rawData)
    {
        KeyFrame p = new KeyFrame
        {
            isValid = (rawData.StartsWith(V_START))
        };

        // If the packet is invalid, finish.
        if (!p.isValid) {
            return p;
        }

        // Parse the other data
        rawData = rawData.Replace(V_START, "");

        string[] data = rawData.Split(',');

        // Pixy position
        p.x = Convert.ToUInt32(data[0]);
        p.y = Convert.ToUInt32(data[1]);
        p.w = Convert.ToUInt32(data[2]);
        p.h = Convert.ToUInt32(data[3]);

        // IR Voltages
        p.irv0 = Convert.ToUInt32(data[4]);
        p.irv1 = Convert.ToUInt32(data[5]);

        // Orientation
        p.heading = float.Parse(data[6]);
        p.pitch = float.Parse(data[7]);
        p.roll = float.Parse(data[8]);

        p.acc = new Vector3(float.Parse(data[9]), float.Parse(data[10]), float.Parse(data[11]));

        // Time
        p.time = ulong.Parse(data[12]);

        return p;
    }

    void UDP_Init()
    {
        remoteEndPoint = new IPEndPoint(IPAddress.Parse(IP), port);
        client = new UdpClient();
        connected = true;
        ui.Connect();
    }

    private static bool pendFlag = false;
    private static bool reconnected = false;

    IEnumerator Reconnect()
    {
        reconnected = false;

        while (!reconnected)
        {
            Debug.Log("Attempting to reconnect...");
            StartCoroutine(ReconnectRequest());

            yield return new WaitForSeconds(2);

            Debug.Log("Reconnection failed.");
            StopCoroutine(ReconnectRequest());

            yield return new WaitForSeconds(2);
        }
    }

    IEnumerator ReconnectRequest()
    {
        // send
        byte[] data = Encoding.UTF8.GetBytes(PREFIX + READY_Q);
        int code = client.Send(data, data.Length, remoteEndPoint);

        yield return null;

        // recieve
        byte[] res = client.Receive(ref remoteEndPoint);
        string response = Encoding.UTF8.GetString(res);

        reconnected = true;
    }

    IEnumerator MakeRequest(string request)
    {
        if (!NO_NET && connected)
        {
            if (pendFlag)
            {
                connected = false;
                pendFlag = false;
                ui.Disconnect();
                //StartCoroutine(Reconnect());
                yield return null;
            }
            else
            {
                pendFlag = true;

                byte[] data = Encoding.UTF8.GetBytes(request);

                double requestTime = Time.time;

                bool error = false;

                try
                {
                    int code = client.Send(data, data.Length, remoteEndPoint);
                }
                catch (Exception e)
                {
                    error = true;
                    Debug.Log("Socket exception");
                }

                if (!error)
                {

                    yield return null;

                    byte[] res = client.Receive(ref remoteEndPoint);

                    pendFlag = false;

                    string response = Encoding.UTF8.GetString(res);

                    double responseTime = Time.time - requestTime;

                    //Debug.Log("Response : " + response);

                    if (opMode == OperatingMode.Animating || opMode == OperatingMode.Calibrating)
                    {
                        if (response.Contains(STOP_S))
                        {
                            if (calibrating)
                                StopCalibrating();
                            else
                                StopAnimating();
                        }
                        else
                        {
                            currentQ.Enqueue(response);
                        }
                    }
                    else
                    {
                        if (response.Contains(READY_R) && !readyQ)
                        {
                            readyQ = true;
                            Debug.Log("Remote button press, waiting for release....");
                        }
                        else if (response.Contains(GO_R) && !goQ)
                        {
                            goQ = true;
                            Debug.Log("Remote button release, BEGIN ANIMATION....");
                        }
                    }

                }
            }
            
        }
    }    
}

public struct KeyFrame
{
    public bool isValid;
    public uint x, y, w, h, irv0, irv1;
    public float heading, pitch, roll;
    public Vector3 acc;
    public ulong time;
}

public class AnimoteAnimation
{
    public ArrayList frames;
    private int FPS;

    public AnimoteAnimation(int FPS)
    {
        this.FPS = FPS;
        frames = new ArrayList();
    }
    public int GetFPS()
    {
        return FPS;
    }

    public ArrayList GetFrames()
    {
        return frames;
    }

    public void AddFrame(KeyFrame frame)
    {
        frames.Add(frame);
    }

}