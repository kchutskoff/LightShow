using System.ComponentModel;
using System.Windows.Forms;

namespace Extensions
{

    public static class InvokeIfRequiredExtension
    {
        public static void InvokeIfRequired(this ISynchronizeInvoke obj, MethodInvoker action)
        {
            if (obj.InvokeRequired)
            {
                var args = new object[0];
                obj.BeginInvoke(action, args);
            }
            else
            {
                action();
            }
        }
    }

}