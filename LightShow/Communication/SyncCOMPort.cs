using System;
using System.Collections.Generic;
using System.IO.Ports;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Threading;

namespace LightShow.Communication
{
    public class SyncCOMPort : IDisposable
    {
        private SerialPort port;
        private object portLock;

        public SyncCOMPort(string portname, int baud)
        {
            this.portLock = new object();

            lock (portLock)
            {
                this.port = new SerialPort(portname, baud);
                port.Open();
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
                this.port.BaseStream.Write(bytes, offset, count);
            }
        }

        public string PortName
        {
            get
            {
                lock (portLock)
                {
                    return port.PortName;
                }
            }
        }

        private int ReadBytes(byte[] buffer, int startOffset, int length)
        {
            lock (portLock)
            {
                return port.BaseStream.Read(buffer, startOffset, length);
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
                    lock (portLock)
                    {
                        if(this.port != null)
                        {
                            this.port.Dispose();
                            this.port = null;
                        }
                    }
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
