using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO.Ports;
using System.Threading;

namespace LightShow.Communication
{
    public class SerialConnection : IDisposable
    {
        private byte[] buffer;
        private SerialPort port;
        private object writeLock;
        private object readLock;
        private ManualResetEvent stopRead;
        private Thread readThread;

        public event OnReadBytesHandler OnReadBytes;


        public delegate void OnReadBytesHandler(object sender, ReadBytesEventArgs EventArgs);

        public class ReadBytesEventArgs : EventArgs
        {
            public byte[] data;

            public ReadBytesEventArgs(byte[] data)
            {
                this.data = data;
            }
        }

        public SerialConnection(string portName, int baudRate, int readBufferSize)
        {
            this.readLock = new object();
            this.writeLock = new object();
            this.port = new SerialPort(portName, baudRate);
            this.buffer = new byte[readBufferSize];
            this.OnReadBytes = null;
            this.stopRead = new ManualResetEvent(false);
        }

        public bool Open()
        {
            lock (writeLock)
            {
                if (this.port.IsOpen == false)
                {
                    try
                    {
                        this.port.Open();
                        this.port.ReadTimeout = 1000;
                        this.port.DiscardInBuffer();
                        this.port.DiscardOutBuffer();
                        if (this.port.IsOpen)
                        {
                            lock (readLock)
                            {
                                this.stopRead.Reset();
                                this.readThread = new Thread(doReading);
                                this.readThread.Name = "Reading Thread";
                                this.readThread.Start();
                            }
                        }
                        return this.port.IsOpen;
                    }
                    catch (Exception)
                    {
                        return false;
                    }
                }
                else
                {
                    return true;
                }
            }
        }

        public void Close()
        {
            lock (writeLock)
            {
                if (this.port.IsOpen)
                {
                    this.stopRead.Set();
                    this.readThread.Join();
                    lock (readLock)
                    {
                        this.readThread = null;
                        this.port.Close();
                    }
                }
            }
        }

        private void doReading()
        {
            while (stopRead.WaitOne(0) == false)
            {
                bool wasCancelled = false;
                int read = 0;
                lock (readLock)
                {
                    if(this.port != null && this.port.IsOpen)
                    {
                        
                        try
                        {
                            this.port.ReadTimeout = 1000;
                            read = this.port.Read(this.buffer, 0, this.buffer.Length);
                        }
                        catch (TimeoutException)
                        {
                            wasCancelled = true;
                        }
                    }else
                    {
                        wasCancelled = true;
                    }
                }
                
                
                if (stopRead.WaitOne(0))
                {
                    return;
                }
                if (wasCancelled)
                {
                    read = 0;
                }
                if (read > 0)
                {
                    byte[] bufferOut = new byte[read];
                    Buffer.BlockCopy(this.buffer, 0, bufferOut, 0, read);
                    this.OnReadBytes?.Invoke(this, new ReadBytesEventArgs(bufferOut));
                }
            }
        }

        public bool WriteBytes(byte[] data, int offset, int length)
        {
            lock (writeLock)
            {
                if (this.port != null && this.port.IsOpen)
                {
                    this.port.Write(data, offset, length);
                    return true;
                }
            }
            return false;
        }

        #region IDisposable Support
        private bool disposedValue = false; // To detect redundant calls

        protected virtual void Dispose(bool disposing)
        {
            if (!disposedValue)
            {
                if (disposing)
                {
                    this.Close();
                    if (this.port != null)
                    {
                        this.port.Dispose();
                        this.port = null;
                    }
                    if(this.stopRead != null)
                    {
                        this.stopRead.Dispose();
                        this.stopRead = null;
                    }
                    this.buffer = null;
                }
                disposedValue = true;
            }
        }

        // This code added to correctly implement the disposable pattern.
        public void Dispose()
        {
            // Do not change this code. Put cleanup code in Dispose(bool disposing) above.
            Dispose(true);
        }
        #endregion
    }
}
