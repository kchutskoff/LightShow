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

        List<Tuple<byte, byte, byte>> colors = new List<Tuple<byte, byte, byte>>();
        int colorIndex = 0;
        Task<bool> waitForAck;
        bool gotAck = false;


        private enum Commands
        {
            ACK = 0,
            ACK_RESP = 1,
            FRM = 2,
            FRM_RESP = 3
        }

        public Main()
        {
            InitializeComponent();
            notifyIcon.Visible = false;
            buttonRefreshAvailPorts_Click(null, null);
            for(uint h = 0; h < 3*256; h+=1)
            {
                byte phase = (byte)(h >> 8);
                byte step = (byte)(h & 0xFF);
                byte rr;
                byte gg;
                byte bb;

                if (phase == 0)
                {
                    rr = (byte)~step;
                    gg = step;
                    bb = 0;
                }else if(phase == 1)
                {
                    rr = 0;
                    gg = (byte)~step;
                    bb = step;
                }else
                {
                    rr = step;
                    gg = 0;
                    bb = (byte)~step;
                }
                colors.Add(new Tuple<byte, byte, byte>((byte)rr, (byte)gg, (byte)bb));
            }
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
                    if (OpenConnection(portName) && SendAck())
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

        private bool OpenConnection(string portName)
        {
            lock (connectionLock)
            {
                //CloseConnection();
                //port = new SerialTransport();
                //port.CurrentSerialSettings.PortName = portName;
                //port.CurrentSerialSettings.Timeout = 1000;
                //port.CurrentSerialSettings.BaudRate = 115200;

                //messenger = new CmdMessenger(port, BoardType.Bit16);
                //messenger.Attach(OnUnknownCommand);
                //messenger.Attach((int)Commands.ACK_RESP, OnAckResponse);
                //messenger.Attach((int)Commands.FRM, OnRequestFrame);

                //bool status = messenger.Connect();
                //if (!status)
                //{
                //    CloseConnection();
                //    return false;
                //}

                CloseConnection();
                com = new Communication.MessageHandler(portName, 115200, 100);
                com.AddMessageHandler((byte)Commands.ACK_RESP, OnAckResponse);
                com.AddMessageHandler((byte)Commands.FRM, OnRequestFrame);
                com.OnExceptionMessage += Com_OnExceptionMessage;

                return com.Connect();
            }
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
            lock (connectionLock)
            {
                if (com != null)
                {
                    com.Disconnect();
                    com.Dispose();
                    com = null;
                }
            }
        }

        private void OnRequestFrame(object sender, Communication.MessageHandler.MessageEventArgs EventArgs)
        {
            this.Invoke((Action)delegate
            {
                textBoxMessages.AppendText("CMD FRM\n");
            });
            byte r = 0x00, g = 0x00, b = 0x00;
            lock (connectionLock)
            {
                if (com != null)
                {
                    byte count = EventArgs.data[0];

                    colorIndex++;
                    if (colorIndex >= colors.Count())
                    {
                        colorIndex = 0;
                    }

                    r = colors[colorIndex].Item1;
                    g = colors[colorIndex].Item2;
                    b = colors[colorIndex].Item3;

                    // one for each color
                    byte[] data = new byte[2 * count];
                    // fill with random values
                    for(var i = 0; i < count; ++i)
                    {
                        data[i * 2] = (byte)((g & 0xF8) | (r >> 5));
                        data[i * 2 + 1] = (byte)(((r & 0xFC) << 3) | (b >> 3));
                    }

                    com.SendMessage((byte)Commands.FRM_RESP, data);
                }
            }
            this.Invoke((Action)delegate
            {
                textBoxMessages.AppendText("SND FRM RESP for " + PrintByteArray(new byte[] { r, g, b }) + " \n");
            });
        }

        private void OnAckResponse(object sender, Communication.MessageHandler.MessageEventArgs args)
        {
            this.Invoke((Action)delegate
            {
                textBoxMessages.AppendText("CMD ACK_RESP\n");
            });
            this.gotAck = true;
        }

        private bool SendAck()
        {
            lock (connectionLock)
            {
                if (com != null)
                {
                    this.gotAck = false;
                    com.SendMessage((byte)Commands.ACK, null);
                    //return System.Threading.SpinWait.SpinUntil(() => this.gotAck == true, 1000);
                    return true;
                }
                return false;
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
