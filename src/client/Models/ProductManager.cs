using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace client.Models
{
    public class ProductManager
    {
        public void Add(Proto.Instrument inst)
        {
            //lock (this.mutex)
            {
                instruments.Add(inst.Id, inst);
            }
        }

        public void Add(IList<Proto.Instrument> instruments)
        {
            //lock (this.mutex)
            {
                foreach (var inst in instruments)
                {
                    this.instruments.Add(inst.Id, inst);
                    if (inst.Type == Proto.InstrumentType.Option)
                    {
                        List<Proto.Instrument> opt = null;
                        if (underlyingOptions.TryGetValue(inst.Underlying, out opt) == false)
                        {
                            opt = new List<Proto.Instrument>();
                            underlyingOptions.Add(inst.Underlying, opt);
                        }
                        opt.Add(inst);
                        if (hedgeUnderlyingOptions.TryGetValue(inst.HedgeUnderlying, out opt) == false)
                        {
                            opt = new List<Proto.Instrument>();
                            hedgeUnderlyingOptions.Add(inst.HedgeUnderlying, opt);
                        }
                        opt.Add(inst);
                    }
                }
            }
        }

        public void Remove(Proto.Instrument inst)
        {
            //lock (this.mutex)
            {
                instruments.Remove(inst.Id);
            }
        }

        public Proto.Instrument FindId(string id)
        {
            Proto.Instrument inst = null;
            //lock (this.mutex)
            {
                instruments.TryGetValue(id, out inst);
            }
            return inst;
        }

        public List<string> GetUnderlyings()
        {
            //lock (this.mutex)
            {
                return underlyingOptions.Keys.ToList();
            }
        }

        public List<Proto.Instrument> GetUnderlyings(string hedgeUnderlying)
        {
            List<Proto.Instrument> ret = new List<Proto.Instrument>();
            foreach (var kvp in instruments)
            {
                if (kvp.Value.Type != Proto.InstrumentType.Option && kvp.Value.HedgeUnderlying == hedgeUnderlying)
                {
                    ret.Add(kvp.Value);
                }
            }
            return ret;
        }

        public List<string> GetHedgeUnderlyings()
        {
            //lock (this.mutex)
            {
                return hedgeUnderlyingOptions.Keys.ToList();
            }
        }

        public List<string> GetHedgeUnderlyings(Proto.Exchange exchange)
        {
            List<string> ret = new List<string>();
            foreach (var kvp in hedgeUnderlyingOptions)
            {
                if (kvp.Value.Count > 0 && kvp.Value[0].Exchange == exchange)
                {
                    ret.Add(kvp.Key);
                }
            }
            return ret;
        }

        public List<Proto.Instrument> GetOptions(string underlying)
        {
            //lock (this.mutex)
            {
                List<Proto.Instrument> ret = null;
                underlyingOptions.TryGetValue(underlying, out ret);
                return ret;
            }
        }

        public List<Proto.Instrument> GetOptionsByHedgeUnderlying(string hedgeUnderlying)
        {
            //lock (this.mutex)
            {
                List<Proto.Instrument> ret = null;
                hedgeUnderlyingOptions.TryGetValue(hedgeUnderlying, out ret);
                return ret;
            }
        }

        //object mutex = new object();
        Dictionary<string, Proto.Instrument> instruments = new Dictionary<string,Proto.Instrument>();
        Dictionary<string, List<Proto.Instrument>> underlyingOptions = new Dictionary<string, List<Proto.Instrument>>();
        Dictionary<string, List<Proto.Instrument>> hedgeUnderlyingOptions = new Dictionary<string, List<Proto.Instrument>>();
    }
}
