using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace LightShow
{
    public partial class Main : Form
    {
        private Communication.MessageHandler com;
        private object connectionLock = new object();

        int hueStart = 0;
        int marqueeStart = 0; // offset of the marquee
        int marqueeSize = 8; // mods the offset value to get the marquee index
        int marqueeSkip = 6; // marquee indexes larger than this are turned off

        byte[] nextFrame = null;

        private bool MarqueePixelIsColored(int i)
        {
            return ((i + marqueeStart) % marqueeSize) < marqueeSkip;
        }


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
            com = new Communication.MessageHandler(portName, 115200, 100);
            com.AddMessageHandler((byte)Commands.FRM, OnRequestFrame);
            com.OnExceptionMessage += Com_OnExceptionMessage;

            return com.Connect();
        }

        private void Com_OnExceptionMessage(object sender, Communication.MessageHandler.MessageExceptionEventArgs EventArgs)
        {
            this.Invoke((Action)delegate
            {
                textBoxMessages.AppendText("ERR: " + EventArgs.messageException.Message + "\n");
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
        }

        private static Color ColorFromHSV(double hue, double saturation, double value)
        {
            int hi = Convert.ToInt32(Math.Floor(hue / 60)) % 6;
            double f = hue / 60 - Math.Floor(hue / 60);

            value = value * 255;
            int v = Convert.ToInt32(value);
            int p = Convert.ToInt32(value * (1 - saturation));
            int q = Convert.ToInt32(value * (1 - f * saturation));
            int t = Convert.ToInt32(value * (1 - (1 - f) * saturation));

            if (hi == 0)
                return Color.FromArgb(255, v, t, p);
            else if (hi == 1)
                return Color.FromArgb(255, q, v, p);
            else if (hi == 2)
                return Color.FromArgb(255, p, v, t);
            else if (hi == 3)
                return Color.FromArgb(255, p, q, v);
            else if (hi == 4)
                return Color.FromArgb(255, t, p, v);
            else
                return Color.FromArgb(255, v, p, q);
        }

        private byte[] BuildNextFrame(int count)
        {
            byte[] data = new byte[3 * count];

            marqueeStart--;
            if (marqueeStart < 0)
            {
                marqueeStart = marqueeSize;
            }

            hueStart++;
            if (hueStart > 360)
            {
                hueStart = 0;
            }

            int deltaHue = 360;
            int curHue = hueStart;

            int errorHue = 2 * deltaHue - count;
            for (int i = 0; i < count; ++i)
            {
                Color ledCol = ColorFromHSV(curHue, 1, 0.6);

                if (MarqueePixelIsColored(i))
                {
                    data[i * 3] = ledCol.G;
                    data[i * 3 + 1] = ledCol.R;
                    data[i * 3 + 2] = ledCol.B;
                }
                else
                {
                    data[i * 3] = 0;
                    data[i * 3 + 1] = 0;
                    data[i * 3 + 2] = 0;
                }

                while (errorHue > 0)
                {
                    curHue += 1;
                    errorHue -= count;
                }
                errorHue += deltaHue;
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
                        byte count = EventArgs.data[0];
                        byte[] data = null;
                        if (nextFrame == null)
                        {
                            data = BuildNextFrame(count);
                        }
                        else
                        {
                            data = nextFrame;
                        }
                        com.SendMessage((byte)Commands.FRM_RESP, data);
                        nextFrame = BuildNextFrame(count);
                    }
                }
                finally
                {
                    System.Threading.Monitor.Exit(connectionLock);
                }
            }
        }

        private string PrintByteArray(byte[] data)
        {
            if(data == null || data.Length == 0)
            {
                return "";
            }
            return "0x" + BitConverter.ToString(data).Replace("-", " 0x");
        }
    }
}
