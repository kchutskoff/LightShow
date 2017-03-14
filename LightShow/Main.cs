using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Diagnostics;
using Extensions;

namespace LightShow
{
    public partial class Main : Form
    {
        private Communication.MessageHandler com;
        private object connectionLock = new object();

        Stopwatch frameTimer = new Stopwatch();
        Queue<long> frameTimings = new Queue<long>();
        const int frameTimingsMax = 5;
        private object fpsLock = new object();
        double framesPerSecond = 0;
        long lastTimestamp = 0;
        Stack<Tuple<double, int>> framesDroppedBelow75 = new Stack<Tuple<double, int>>();
        byte lastframeid = 0;
        Queue<byte> frameIDs = new Queue<byte>();

        UInt32 currentDataSize = 0;

        private enum Commands
        {
            FRM = 0,
            FRM_RESP = 1
        }

        public Main()
        {
            InitializeComponent();
            notifyIcon.Visible = false;
            buttonRefreshAvailPorts_Click(null, null);

        }

        private void Main_Resize(object sender, EventArgs e)
        {
            if(this.WindowState == FormWindowState.Minimized)
            {
                notifyIcon.Visible = true;
                this.ShowInTaskbar = false;
            }
        }

        private void notifyIcon_Click(object sender, EventArgs e)
        {
            this.WindowState = FormWindowState.Normal;
            this.ShowInTaskbar = true;
            notifyIcon.Visible = false;

        }

        private void buttonRefreshAvailPorts_Click(object sender, EventArgs e)
        {
            string selectedItem = (string)comboBoxPort.SelectedItem ?? "";
            comboBoxPort.Items.Clear();
            comboBoxPort.Items.AddRange(System.IO.Ports.SerialPort.GetPortNames());
            if(comboBoxPort.Items.Count > 0)
            {
                comboBoxPort.Enabled = true;
                if (comboBoxPort.Items.Contains(selectedItem))
                {
                    comboBoxPort.SelectedIndex = comboBoxPort.Items.IndexOf(selectedItem);
                }else
                {
                    comboBoxPort.SelectedIndex = 0;
                }
            }else
            {
                comboBoxPort.Enabled = false;
            }
        }

        private void buttonConnect_Click(object sender, EventArgs e)
        {
            buttonConnect.Enabled = false;
            lock (connectionLock)
            {
                if (com != null)
                {
                    CloseConnection();
                    labelStatus.Text = "Disconnected";
                    buttonConnect.Text = "Connect";
                    buttonConnect.Enabled = true;
                }
                else
                {
                    string portName = (string)comboBoxPort.SelectedItem ?? "";
                    if (String.IsNullOrEmpty(portName))
                    {
                        MessageBox.Show("A valid port must be selected!", "Unable to Connect", MessageBoxButtons.OK, MessageBoxIcon.Error);
                        buttonConnect.Enabled = true;
                    }
                    else
                    {
                        if (OpenConnection(portName))
                        {
                            labelStatus.Text = "Connected";
                            buttonConnect.Text = "Disconnect";
                            buttonConnect.Enabled = true;
                        }
                        else
                        {
                            CloseConnection();
                            labelStatus.Text = "Unable to connect";
                            buttonConnect.Text = "Connect";
                            buttonConnect.Enabled = true;
                        }
                    }
                }
            }
        }

        private bool OpenConnection(string portName)
        {
            CloseConnection();
            com = new Communication.MessageHandler(portName, 1000000, 1024);
            com.AddMessageHandler((byte)Commands.FRM, OnRequestFrame);
            com.OnExceptionMessage += Com_OnExceptionMessage;

            frameTimings.Clear();
            frameTimer.Restart();
            framesPerSecond = 0;

            return com.Connect();
        }

        private void Com_OnExceptionMessage(object sender, Communication.MessageHandler.MessageExceptionEventArgs EventArgs)
        {
            textBoxMessages.InvokeIfRequired(() => {
                textBoxMessages.AppendText(DateTime.Now.ToString("HH:mm:ss: ") + EventArgs.messageException.Message + "\r\n");
            });
        }

        private void CloseConnection()
        {
            if (com != null)
            {
                com.Disconnect();
                com.Dispose();
                com = null;
            }

            frameTimer.Reset();
        }


        private byte[] BuildNextFrame(int count)
        {
            byte[] data = new byte[count + 1];
            for (int i = 1; i < count + 1; ++i)
            {
                if (i % 2 == 0)
                {
                    data[i] = 0xCA;
                }
                else
                {
                    data[i] = 0x35;
                }
            }
            return data;
        }

        private void OnRequestFrame(object sender, Communication.MessageHandler.MessageEventArgs EventArgs)
        {
            if(System.Threading.Monitor.TryEnter(connectionLock))
            {
                try
                {
                    if (com != null)
                    {
                        if(EventArgs.data.Length >= 3)
                        {
                            byte frameid = EventArgs.data[0];
                            byte high_count = EventArgs.data[1];
                            byte low_count = EventArgs.data[2];
                            UInt16 count = (UInt16)((high_count << 8) | low_count);
                            byte[] data = BuildNextFrame(count);
                            data[0] = frameid;
                            com.SendMessage((byte)Commands.FRM_RESP, data);

                            long now = frameTimer.ElapsedTicks;
                            if (frameTimings.Count > 0)
                            {
                                long difference = now - frameTimings.Peek();
                                long ticksPerFrame = difference / frameTimings.Count;
                                double secondsPerFrame = ((double)ticksPerFrame / Stopwatch.Frequency);
                                double fps = 1 / secondsPerFrame;

                                difference = now - lastTimestamp;
                                secondsPerFrame = ((double)difference / Stopwatch.Frequency);
                                double fpslast = 1 / secondsPerFrame;

                                lock (fpsLock)
                                {
                                    frameIDs.Enqueue(frameid);
                                    currentDataSize = count;
                                    framesPerSecond = fps;
                                    if (fpslast < fps * 0.75)
                                    {
                                        framesDroppedBelow75.Push(new Tuple<double, int>(fpslast, count));
                                    }
                                }
                            }
                            lastTimestamp = now;
                            frameTimings.Enqueue(now);
                            while (frameTimings.Count > frameTimingsMax)
                            {
                                frameTimings.Dequeue();
                            }
                        }
                        else
                        {
                            throw new Exception("Expected 2 bytes in packet and only got " + EventArgs.data.Length + "!");
                        }
                    }
                }
                finally
                {
                    System.Threading.Monitor.Exit(connectionLock);
                }
            }
            this.labelFPS.InvokeIfRequired(UpdateFramesPerSecond);
        }

        private void UpdateFramesPerSecond()
        {
            lock (fpsLock)
            {
                this.labelFPS.Text = this.framesPerSecond.ToString("00.00") + " [" + currentDataSize + "]";
                while(framesDroppedBelow75.Count > 0)
                {
                    Tuple<double, int> item = framesDroppedBelow75.Pop();
                    textBoxMessages.AppendText(DateTime.Now.ToString("HH:mm:ss: ") + "Frame dropped below 75% of average (fps " + item.Item1.ToString("00.00") + ", data count: " + item.Item2.ToString() + ")\r\n");
                }
                while(frameIDs.Count > 0)
                {
                    byte curID = frameIDs.Dequeue();
                    lastframeid++;
                    if(curID != lastframeid)
                    {
                        textBoxMessages.AppendText(DateTime.Now.ToString("HH:mm:ss: ") + "Frame ID " + PrintByteArray(lastframeid) + " was dropped!\r\n");
                        lastframeid = curID;
                    }else
                    {
                        textBoxMessages.AppendText(DateTime.Now.ToString("HH:mm:ss: ") + "Frame ID " + PrintByteArray(lastframeid) + " = SUCCESS!\r\n");
                    }
                }
            }
            
        }


        private string PrintByteArray(params byte[] data)
        {
            if(data == null || data.Length == 0)
            {
                return "";
            }
            return "0x" + BitConverter.ToString(data).Replace("-", " 0x");
        }
    }
}
