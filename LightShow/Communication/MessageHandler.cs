using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace LightShow.Communication
{
    public class MessageHandler : IDisposable
    {
        public delegate void OnMessageHandler(object sender, MessageEventArgs EventArgs);
        public delegate void OnMessageExceptionHandler(object sender, MessageExceptionEventArgs EventArgs);

        public class MessageEventArgs : EventArgs
        {
            public byte[] data;

            public MessageEventArgs(byte[] data)
            {
                this.data = data;
            }
        }

        public class MessageExceptionEventArgs : EventArgs
        {
            public Exception messageException;

            public MessageExceptionEventArgs(Exception e)
            {
                this.messageException = e;
            }
        }

        private SerialConnection serial;
        private byte[] buffer;
        private int bufferIndex;
        private Dictionary<byte, OnMessageHandler> messageCallbacks;
        public event OnMessageExceptionHandler OnExceptionMessage;

        public MessageHandler(string portName, int baudRate, int bufferSize)
        {
            this.messageCallbacks = new Dictionary<byte, OnMessageHandler>();
            this.OnExceptionMessage = null;
            this.serial = new SerialConnection(portName, baudRate, bufferSize/4);
            this.serial.OnReadBytes += Serial_OnReadBytes;
            this.buffer = new byte[bufferSize];
            this.bufferIndex = 0;
        }

        public bool Connect()
        {
            return this.serial.Open();
        }

        public void Disconnect()
        {
            this.serial.Close();
        }

        public void AddMessageHandler(byte message, OnMessageHandler handler)
        {
            if(messageCallbacks.ContainsKey(message) == false)
            {
                messageCallbacks[message] = handler;
            }
            else
            {
                messageCallbacks[message] += handler;
            }
            
        }

        public bool SendMessage(byte messageType, byte[] data)
        {
            data = EscapeStream(data);
            byte[] output = new byte[data.Length + 5];
            output[0] = 0x55;
            output[1] = 0xFF;
            output[2] = messageType;
            output[3 + data.Length] = 0x55;
            output[4 + data.Length] = 0x00;
            Buffer.BlockCopy(data, 0, output, 3, data.Length);
            return this.serial.WriteBytes(output, 0, output.Length);
        }

        private void Serial_OnReadBytes(object sender, SerialConnection.ReadBytesEventArgs EventArgs)
        {
            Buffer.BlockCopy(EventArgs.data, 0, buffer, bufferIndex, EventArgs.data.Length);
            bufferIndex += EventArgs.data.Length;
            int packetStart = 0;
            int packetEnd = 0;

            while(GetNextPacket(ref packetStart, out packetEnd))
            {
                ParsePacket(packetStart, packetEnd);
                packetStart = packetEnd;
            }
            // we can delete up to packetstart
            Buffer.BlockCopy(buffer, packetStart, buffer, 0, (bufferIndex - packetStart));
            bufferIndex = (bufferIndex - packetStart);
        }

        private bool GetNextPacket(ref int packetStart, out int packetEnd) {
            int reader = packetStart;
            packetStart = bufferIndex;
            packetEnd = -1;
            bool foundStart = false;
            while(reader < bufferIndex)
            {
                if(buffer[reader] == 0x55)
                {
                    if (reader + 1 < bufferIndex)
                    {
                        if (buffer[reader + 1] == 0xFF)
                        {
                            // found start
                            packetStart = reader;
                            foundStart = true;
                            reader++; // extra to skip over 0xFF
                        } else if (buffer[reader + 1] == 0x00)
                        {
                            // found end
                            if (foundStart)
                            {
                                packetEnd = reader + 2;
                                return true;
                            }
                        } else if (buffer[reader + 1] != 0xFE)
                        {
                            RaiseError(new Exception("Received unescaped 0x55 byte in input stream!"));
                            foundStart = false;
                            packetStart = bufferIndex;
                        }
                    }else
                    {
                        // can't read rest
                        packetStart = reader;
                    }
                }
                reader++;
            }
            return false;
        }

        private void ParsePacket(int start, int end)
        {
            if(buffer[start] != 0x55 && buffer[start + 1] != 0xFF)
            {
                RaiseError(new Exception("Illegal start for packet! Expected 0x55, 0xFF and got " + printByte(buffer[start]) + ", " + printByte(buffer[start + 1]) + "!"));
                return;
            }
            if(buffer[end - 2] != 0x55 && buffer[end - 1] != 0x00)
            {
                RaiseError(new Exception("Illegal end for packet! Expected 0x55, 0x00 and got " + printByte(buffer[start]) + ", " + printByte(buffer[start + 1]) + "!"));
                return;
            }
            if(end - start < 5)
            {
                RaiseError(new Exception("Illegal packet size. Expected at least one byte between start and end of packet!"));
                return;
            }
            byte messageType = buffer[start + 2];
            OnMessageHandler handler = null;
            if(messageCallbacks.TryGetValue(messageType, out handler))
            {
                byte[] data = new byte[end - start - 5];
                Buffer.BlockCopy(buffer, start + 3, data, 0, end - start - 5);
                data = UnescapeStream(data);
                handler?.Invoke(this, new MessageEventArgs(data));
            }else
            {
                RaiseError(new Exception("Unknown message type " + printByte(messageType) + " received on input stream!"));
            }
        }

        private void RaiseError(Exception e)
        {
            this.OnExceptionMessage?.Invoke(this, new MessageExceptionEventArgs(e));
        }

        public static byte[] EscapeStream(byte[] data)
        {
            if(data == null)
            {
                return new byte[0];
            }
            List<int> escapeIndexes = new List<int>();
            for(var i= 0; i < data.Length; ++i)
            {
                if(data[i] == 0x55)
                {
                    escapeIndexes.Add(i);
                }
            }
            // check if nothing to escape
            if(escapeIndexes.Count() == 0)
            {
                return data;
            }
            // make a new array
            byte[] output = new byte[data.Length + escapeIndexes.Count()];
            int lastIndex = 0;
            // for each escape index
            for (var offset = 0; offset < escapeIndexes.Count(); ++offset)
            {
                int nextIndex = escapeIndexes[offset];
                // add everything up to and including the next escape character
                Buffer.BlockCopy(data, lastIndex, output, lastIndex + offset, (nextIndex - lastIndex + 1));
                // add the new escape char
                output[nextIndex + offset + 1] = 0xFE;
                // record for next loop
                lastIndex = nextIndex + 1;
            }
            // last char wasn't escaped, missing rest, so add it now
            if(lastIndex < data.Length)
            {
                Buffer.BlockCopy(data, lastIndex, output, lastIndex + escapeIndexes.Count(), (data.Length - lastIndex));
            }
            return output;
        }

        public static byte[] UnescapeStream(byte[] data)
        {
            if(data == null)
            {
                return new byte[0];
            }
            List<int> escapedIndexes = new List<int>();
            for (var i = 0; i < data.Length; ++i)
            {
                if (data[i] == 0x55)
                {
                    escapedIndexes.Add(i);
                }
            }
            // check if nothing to unescape
            if (escapedIndexes.Count() == 0)
            {
                return data;
            }
            // make a new array
            byte[] output = new byte[data.Length - escapedIndexes.Count()];
            int lastIndex = 0;
            // for each escaped index
            for (var offset = 0; offset < escapedIndexes.Count(); ++offset)
            {
                int nextIndex = escapedIndexes[offset];
                // add everything up to and including the next escaped character
                Buffer.BlockCopy(data, lastIndex, output, lastIndex - offset, (nextIndex - lastIndex + 1));
                if(data[nextIndex + 1] != 0xFE)
                {
                    throw new Exception("Incorrect escape character after 0x55! Expected 0xFE and got " + printByte(data[nextIndex + 1]) + "!");
                }
                // record for next loop, skip FE
                lastIndex = nextIndex + 2;
            }
            // last char wasn't escaped, missing rest, so add it now
            if (lastIndex < data.Length)
            {
                Buffer.BlockCopy(data, lastIndex, output, lastIndex - escapedIndexes.Count(), (data.Length - lastIndex));
            }
            return output;
        }

        private static string printByte(byte b)
        {
            return "0x" + BitConverter.ToString(new byte[] { b });
        }

        #region IDisposable Support
        private bool disposedValue = false; // To detect redundant calls

        protected virtual void Dispose(bool disposing)
        {
            if (!disposedValue)
            {
                if (disposing)
                {
                    this.Disconnect();
                    this.serial.Dispose();
                    this.serial = null;
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
