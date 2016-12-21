using System;
using System.Collections.Generic;
using System.IO.Ports;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Threading;

namespace LightShow.Communication
{
    public class AsyncCOMPort : IDisposable
    {
        public delegate void OnReadBytesHandler(object sender, AsyncCOMPortEventArgs EventArgs);

        private SerialPort port;
        private byte[] buffer;
        private CancellationTokenSource stopReadToken;
        public OnReadBytesHandler OnReadBytes;
        private Task<int> readTask;
        private Task afterReadTask;
        private object portLock;

        public class AsyncCOMPortEventArgs : EventArgs
        {
            public byte[] data;

            public AsyncCOMPortEventArgs(byte[] data)
            {
                this.data = data;
            }
        }

        public AsyncCOMPort(string portname, int baud, int bufferSize, OnReadBytesHandler handler)
        {
            this.portLock = new object();
            
            lock (portLock)
            {
                System.Diagnostics.Debug.WriteLine("in lock A");
                this.buffer = new byte[bufferSize];
                this.port = new SerialPort(portname, baud);
                this.OnReadBytes += handler;
                this.stopReadToken = null;
                this.readTask = null;
                this.afterReadTask = null;
                port.Open();
                beginRead();
                System.Diagnostics.Debug.WriteLine("out lock A");
            }
        }

        public void WriteBytes(byte[] bytes)
        {
            WriteBytes(bytes, 0, bytes.Length);
        }

        public void WriteBytes(byte[] bytes, int offset, int count)
        {
            lock (portLock)
            {
                System.Diagnostics.Debug.WriteLine("in lock B");
                this.port.BaseStream.Write(bytes, offset, count);
                System.Diagnostics.Debug.WriteLine("out lock B");
            }
        }

        public string PortName
        {
            get
            {
                lock (portLock)
                {
                    System.Diagnostics.Debug.WriteLine("in/out lock D");
                    return port.PortName;
                }
            }
        }

        private void stopRead()
        {
            lock (portLock)
            {
                System.Diagnostics.Debug.WriteLine("in lock E");
                if (this.stopReadToken != null)
                {
                    this.stopReadToken.Cancel();
                    if (this.readTask != null)
                    {
                        try
                        {
                            this.readTask.Wait();
                        }catch(AggregateException) { }
                        this.readTask.Dispose();
                        this.readTask = null;
                    }
                    if (this.afterReadTask != null)
                    {
                        try
                        {
                            this.afterReadTask.Wait();
                        }
                        catch (AggregateException) { }
                        this.afterReadTask.Dispose();
                        this.afterReadTask = null;
                    }
                    this.stopReadToken.Dispose();
                    this.stopReadToken = null;
                }
                System.Diagnostics.Debug.WriteLine("out lock E");
            }
        }

        private void beginRead()
        {
            lock (portLock)
            {
                System.Diagnostics.Debug.WriteLine("in lock G");
                this.stopRead();
                this.stopReadToken = new System.Threading.CancellationTokenSource();
                this.readTask = port.BaseStream.ReadAsync(buffer, 0, buffer.Length, stopReadToken.Token);
                this.afterReadTask = this.readTask.ContinueWith(onRead, stopReadToken.Token);
                System.Diagnostics.Debug.WriteLine("out lock G");
            }
        }

        private void continueRead()
        {
            lock (portLock)
            {
                if(this.stopReadToken != null && this.stopReadToken.IsCancellationRequested == false)
                {
                    this.readTask = this.afterReadTask.ContinueWith(startNextRead, stopReadToken.Token);
                    this.afterReadTask = this.readTask.ContinueWith(onRead, stopReadToken.Token);
                }
            }
        }

        private int startNextRead(Task obj)
        {
            Task<int> innerTask = port.BaseStream.ReadAsync(buffer, 0, buffer.Length, stopReadToken.Token);
            try
            {
                innerTask.Wait();
            }
            catch (AggregateException)
            {
                return 0;
            }
            return innerTask.Result;
        }

        private void onRead(Task<int> obj)
        {
            lock (portLock)
            {
                System.Diagnostics.Debug.WriteLine("in lock H");
                if (obj.Result > 0)
                {
                    byte[] readData = new byte[obj.Result];
                    Buffer.BlockCopy(buffer, 0, readData, 0, obj.Result);
                    this.OnReadBytes?.Invoke(this, new AsyncCOMPortEventArgs(readData));
                }
                if (!this.stopReadToken.IsCancellationRequested)
                {
                    continueRead();
                }
                System.Diagnostics.Debug.WriteLine("out lock H");
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
                    stopRead();
                    this.OnReadBytes = null;
                }

                disposedValue = true;
            }
        }

        // This code added to correctly implement the disposable pattern.
        public void Dispose()
        {
            Dispose(true);
        }
        #endregion
    }
}
