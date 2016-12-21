using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO.Ports;
using System.Collections.ObjectModel;
using System.Threading;

namespace LightShow.Communication
{
    public class LightShowCOM : IDisposable
    {
        

        private AsyncCOMPort port;
        private object portLock;

        private static readonly byte[] HEADER_FORMAT = { 0x55, 0xFF };
        private static readonly byte[] ACK_FORMAT = { 0x01, 0xFF, 0x0F, 0xF0 };
        private static readonly byte[] ACK_RESPONSE_FORMAT = { 0x02, 0x0F, 0xF0, 0xFF };
        private static readonly byte ESCPAE_HEADER_START = 0x00;

        public LightShowCOM()
        {
            port = null;
            portLock = new object();
        }

        public string PortName
        {
            get
            {
                lock (portLock)
                {
                    if (port == null)
                    {
                        return null;
                    }
                    else
                    {
                        return port.PortName;
                    }
                }
            }
        }

        public void Disconnect()
        {
            lock (portLock)
            {
                if (port != null)
                {
                    port.Dispose();
                    port = null;
                }
            }
        }

        public void FindRefresh()
        {
            Disconnect();
            string[] portNames = SerialPort.GetPortNames();
            Parallel.ForEach(portNames, (portName) =>
            {
                AsyncCOMPort tempPort = null;
                bool dispose = true;
                try
                {
                    byte[] ackBuffer = new byte[100];
                    int bufferIndex = 0;
                    System.Diagnostics.Debug.WriteLine("Listening on Port " + portName + "...");
                    AsyncCOMPort.OnReadBytesHandler handler = null;
                    CancellationTokenSource cancelTimeout = new CancellationTokenSource();
                    Task timeoutPort = Task.Delay(10000, cancelTimeout.Token);
                    handler = (object sender, AsyncCOMPort.AsyncCOMPortEventArgs args) =>
                    {
                        AsyncCOMPort thisPort = (AsyncCOMPort)sender;
                        System.Diagnostics.Debug.WriteLine("Received " + args.data.Length + " bytes on Port " + portName + "...");
                        // we got some data, but we likely also have some data in our buffer from before, so allocate a new buffer of both sizes, and write both to that for the logic
                        byte[] tempBuffer = new byte[bufferIndex + args.data.Length];
                        Buffer.BlockCopy(ackBuffer, 0, tempBuffer, 0, bufferIndex);
                        Buffer.BlockCopy(args.data, 0, tempBuffer, bufferIndex, args.data.Length);
                        
                        bufferIndex += args.data.Length;
                        System.Diagnostics.Debug.WriteLine("Read buffer now contains " + bufferIndex + " bytes for Port " + portName + "...");
                        // now have a contiguous collection of all the data received, try find the ack
                        int lastPacketStart = 0;
                        int readTo = 0;
                        bool haveAck = false;
                        while(readTo < bufferIndex)
                        {
                            lastPacketStart = FindPacketStartIndex(tempBuffer, lastPacketStart);
                            if(lastPacketStart < 0)
                            {
                                // not found, we can drop up to negative items
                                lastPacketStart = -lastPacketStart;
                                readTo = bufferIndex;
                                System.Diagnostics.Debug.WriteLine("No packet header found up to " + lastPacketStart + " for Port " + portName + "...");
                            }
                            else
                            {
                                System.Diagnostics.Debug.WriteLine("Found packet header at " + lastPacketStart + " for Port " + portName + "...");
                                // found start
                                if (lastPacketStart + HEADER_FORMAT.Length + ACK_FORMAT.Length < bufferIndex)
                                {
                                    System.Diagnostics.Debug.WriteLine("Checking ACK for Port " + portName + "...");
                                    // have enough data to read the ack
                                    bool isAck = true;
                                    for(var i = 0; i < ACK_FORMAT.Length; ++i)
                                    {
                                        if(ACK_FORMAT[i] != tempBuffer[i + lastPacketStart + HEADER_FORMAT.Length])
                                        {
                                            isAck = false;
                                            break;
                                        }
                                    }
                                    if (isAck)
                                    {
                                        System.Diagnostics.Debug.WriteLine("ACK Found for Port " + portName + "...");
                                        // we have an ack, return
                                        haveAck = true;
                                        readTo = bufferIndex;
                                    }
                                    else
                                    {
                                        System.Diagnostics.Debug.WriteLine("ACK not found for Port " + portName + "...");
                                        // didn't get it, keep moving on
                                        lastPacketStart += HEADER_FORMAT.Length;
                                        readTo = lastPacketStart;
                                    }
                                }else
                                {
                                    System.Diagnostics.Debug.WriteLine("Not enough read data for ACK on " + portName + "...");
                                    // not enough to read, wait for more data
                                    readTo = bufferIndex;
                                }
                            }
                        }
                        if (haveAck)
                        {
                            // try allocate as main port
                            lock (portLock)
                            {
                                if (port == null)
                                {
                                    System.Diagnostics.Debug.WriteLine("Set " + portName + " as main Port...");
                                    // assign port to main port
                                    port = thisPort;
                                    port.OnReadBytes -= handler;
                                    port.OnReadBytes += onDataRead;
                                    // send the response ack so we notify that we have connected
                                    byte[] sendBuffer = new byte[HEADER_FORMAT.Length + ACK_RESPONSE_FORMAT.Length];
                                    Buffer.BlockCopy(HEADER_FORMAT, 0, sendBuffer, 0, HEADER_FORMAT.Length);
                                    Buffer.BlockCopy(ACK_RESPONSE_FORMAT, 0, sendBuffer, HEADER_FORMAT.Length, ACK_RESPONSE_FORMAT.Length);
                                    port.WriteBytes(sendBuffer);
                                    dispose = false;
                                    cancelTimeout.Cancel();
                                }
                                else
                                {
                                    System.Diagnostics.Debug.WriteLine("Already have a main port, discarding " + portName + "...");
                                }
                            }
                        }else
                        {
                            // drop everything up to lastPacketStart for next data to come in
                            int bytesToKeep = (bufferIndex - lastPacketStart);
                            System.Diagnostics.Debug.WriteLine("Keeping " + bytesToKeep + " in read buffer for " + portName + "...");
                            Buffer.BlockCopy(tempBuffer, lastPacketStart, ackBuffer, 0, bytesToKeep);
                            bufferIndex = bytesToKeep;
                        }                        
                    };
                    tempPort = new AsyncCOMPort(portName, 9600, 50, handler);
                    try
                    {
                        timeoutPort.Wait();
                    }
                    catch (AggregateException) { }
                }
                catch (Exception e) {
                    string msg = e.Message;
                    while(e.InnerException != null)
                    {
                        msg += "\nInner: " + e.InnerException.Message;
                        e = e.InnerException;
                    }
                    System.Diagnostics.Debug.WriteLine("Error when connecting to " + portName + ": " + msg);
                }
                finally
                {
                    if (port != null && dispose)
                    {
                        port.Dispose();
                        port = null;
                    }
                }
            });
        }

        private void onDataRead(object sender, AsyncCOMPort.AsyncCOMPortEventArgs args)
        {

        }

        private int FindPacketStartIndex(byte[] data, int startFrom)
        {
            for(int i = startFrom; i < data.Length; ++i)
            {
                // first byte matches
                if(data[i] == HEADER_FORMAT[0])
                {
                    // have room to check and it matches rest of header, return index
                    if(i < data.Length - 1) {
                        // have room to check
                        if (data[i + 1] == HEADER_FORMAT[1])
                        {
                            return i;
                        }else if(data[i + 1] != ESCPAE_HEADER_START)
                        {
                            throw new Exception("Unescaped byte value 0x55 in message stream!");
                        }                            
                    }else
                    {
                        // can't check, return negative index to say it may be coming up
                        return -i;
                    }
                }
            }
            // not there, return negative length of data to say we passed
            return -data.Length;
        }

        private bool tryHandshake(SerialPort port)
        {
            try
            {
                System.Diagnostics.Debug.WriteLine("Sending request...");
                port.ReadTimeout = 1000;
                port.DiscardInBuffer();
                port.DiscardOutBuffer();
                port.Write("\nLightShow\n");
                string response = port.ReadLine();
                System.Diagnostics.Debug.WriteLine("Got response \"" + response + "\" from " + port.PortName + "...");
                if(response == "LightsOn")
                {
                    return true;
                }else
                {
                    return false;
                }
            }
            catch (TimeoutException)
            {
                System.Diagnostics.Debug.WriteLine("No response from " + port.PortName + "...");
                return false;
            }
            
        }


        #region IDisposable Support
        private bool disposedValue = false; // To detect redundant calls

        protected virtual void Dispose(bool disposing)
        {
            if (!disposedValue)
            {
                if (disposing)
                {
                    if(port != null)
                    {
                        port.Dispose();
                        port = null;
                    }
                }

                disposedValue = true;
            }
        }

        public void Dispose()
        {
            Dispose(true);
        }
        #endregion


    }
}
