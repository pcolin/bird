using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace client.Models
{
    class StrategySwitchManager
    {
        public void OnProtoMessage(Proto.StrategySwitchRep rep)
        {
            //lock (this.mutex)
            //{
            //    //foreach (var s in rep.Switches)
            //    //{
            //    //    switches[(int)s.Strategy][s.Option] = s;
            //    //}
            //}
            lock (this.switches)
            {
                foreach (var s in rep.Switches)
                {
                    Proto.StrategySwitch[] sw = null;
                    if (this.switches.TryGetValue(s.Option, out sw) == false)
                    {
                        sw = new Proto.StrategySwitch[(int)Proto.StrategyType.DummyQuoter + 1];
                        this.switches[s.Option] = sw;
                    }
                    sw[(int)s.Strategy] = s;
                }
            }
        }

        public void OnProtoMessage(Proto.StrategySwitchReq req)
        {
            lock (this.switches)
            {
                foreach (var s in req.Switches)
                {
                    Proto.StrategySwitch[] sw = null;
                    if (switches.TryGetValue(s.Option, out sw) == false)
                    {
                        sw = new Proto.StrategySwitch[] { null, null, null, null };
                        switches.Add(s.Option, sw);
                    }
                    sw[(int)s.Strategy] = s;
                }
            }
        }

        public Proto.StrategySwitch[] GetStrategySwitch(string option)
        {
            lock (this.switches)
            {
                Proto.StrategySwitch[] sw = null;
                switches.TryGetValue(option, out sw);
                return sw;
            }
        }

        public Proto.StrategySwitch GetStrategySwitch(Proto.StrategyType strategy, string option)
        {
            lock (this.switches)
            {
                Proto.StrategySwitch[] sw = null;
                if (switches.TryGetValue(option, out sw))
                {
                    return sw[(int)strategy];
                }
            }
            return null;
        }
        //public void OnProtoMessage(Proto.StrategySwitchReq req)
        //{
        //    lock (this.mutex)
        //    {
        //        if (req.Type == Proto.RequestType.Set)
        //        {
        //            foreach (var s in req.Switches)
        //            {
        //                switches[(int)s.Strategy][s.Option] = s;
        //            }
        //        }
        //        else if (req.Type == Proto.RequestType.Del) { }
        //    }
        //}

        //public Proto.StrategySwitch GetStrategySwitch(Protostring instrument)
        //{
        //    lock (this.mutex)
        //    {
        //        Proto.Position p = null;
        //        if (positions.TryGetValue(instrument, out p))
        //        {
        //            return p.TotalLong - p.TotalShort;
        //        }
        //    }
        //    return 0;
        //}

        //Dictionary<string, Proto.StrategySwitch>[] switches = new Dictionary<string, Proto.StrategySwitch>[3]
        //    {
        //        new Dictionary<string, Proto.StrategySwitch>(),
        //        new Dictionary<string, Proto.StrategySwitch>(),
        //        new Dictionary<string, Proto.StrategySwitch>(),
        //    };
        //private object mutex = new object();

        //Proto.StrategySwitchRep rep = null;
        Dictionary<string, Proto.StrategySwitch[]> switches = new Dictionary<string,Proto.StrategySwitch[]>();
    }
}
