using System;
using System.Collections.Generic;
using System.Diagnostics.Tracing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace client.Models
{
    [EventSource(Name = "Bird-Client")]
    public sealed class LoggingEventSource : EventSource
    {
        public static LoggingEventSource Log = new LoggingEventSource();

        [Event(1, Level = EventLevel.Critical)]
        public void Fatal(string msg)
        {
            WriteEvent(1, msg);
        }

        [Event(2, Level = EventLevel.Error)]
        public void Error(string msg)
        {
            WriteEvent(2, msg);
        }

        [Event(3, Level = EventLevel.Warning)]
        public void Warn(string msg)
        {
            WriteEvent(3, msg);
        }

        [Event(4, Level = EventLevel.Informational)]
        public void Info(string msg)
        {
            WriteEvent(4, msg);
        }

        [Event(5, Level = EventLevel.Verbose)]
        public void Debug(string msg)
        {
            WriteEvent(5, msg);
        }
    }
}
