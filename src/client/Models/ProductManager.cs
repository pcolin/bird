using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace client.Models
{
    public class ProductManager
    {
        public void OnProtoMessage(Proto.InstrumentRep rep)
        {
            var instrumentUnderlyings = new Dictionary<string, Tuple<Instrument, string, string>>();
            foreach (var inst in rep.Instruments)
            {
                try
                {
                    var maturity = DateTime.ParseExact(inst.Maturity, "yyyyMMdd", CultureInfo.InvariantCulture);
                    Instrument instrument = null;
                    if (inst.Type == Proto.InstrumentType.Option)
                    {
                        var option = new Option();
                        option.OptionType = inst.CallPut;
                        option.ExerciseType = inst.Exercise;
                        option.SettlementType = inst.Settlement;
                        option.Strike = inst.Strike;
                        instrument = option;
                    }
                    else
                    {
                        instrument = new Instrument();
                    }
                    instrument.Id = inst.Id;
                    instrument.Symbol = inst.Symbol;
                    instrument.Status = inst.Status;
                    instrument.Exchange = inst.Exchange;
                    instrument.Type = inst.Type;
                    instrument.Currency = inst.Currency;
                    instrument.Tick = inst.Tick;
                    instrument.Multiplier = inst.Multiplier;
                    instrument.Highest = inst.Highest;
                    instrument.Lowest = inst.Lowest;
                    instrument.Maturity = maturity;
                    //this.instruments.Add(inst.Id, instrument);
                    instrumentUnderlyings.Add(inst.Id, Tuple.Create(instrument, inst.Underlying, inst.HedgeUnderlying));
                }
                catch (Exception) {}
            }

            foreach (var kvp in instrumentUnderlyings)
            {
                var inst = kvp.Value.Item1;
                inst.Underlying = instrumentUnderlyings[kvp.Value.Item2].Item1;
                inst.HedgeUnderlying = instrumentUnderlyings[kvp.Value.Item3].Item1;
                this.instruments.Add(kvp.Key, inst);
                if (inst.Type == Proto.InstrumentType.Option)
                {
                    var option = inst as Option;
                    List<Option> options = null;
                    if (underlyingOptions.TryGetValue(inst.Underlying, out options) == false)
                    {
                        options = new List<Option>();
                        underlyingOptions.Add(inst.Underlying, options);
                    }
                    options.Add(option);
                    if (hedgeUnderlyingOptions.TryGetValue(inst.HedgeUnderlying, out options) == false)
                    {
                        options = new List<Option>();
                        hedgeUnderlyingOptions.Add(inst.HedgeUnderlying, options);
                    }
                    options.Add(option);
                }
            }
        }

        //public void Add(Proto.Instrument inst)
        //{
        //    //lock (this.mutex)
        //    {
        //        instruments.Add(inst.Id, inst);
        //    }
        //}

        //public void Add(IList<Proto.Instrument> instruments)
        //{
        //    //lock (this.mutex)
        //    {
        //        foreach (var inst in instruments)
        //        {
        //            this.instruments.Add(inst.Id, inst);
        //            if (inst.Type == Proto.InstrumentType.Option)
        //            {
        //                List<Proto.Instrument> opt = null;
        //                if (underlyingOptions.TryGetValue(inst.Underlying, out opt) == false)
        //                {
        //                    opt = new List<Proto.Instrument>();
        //                    underlyingOptions.Add(inst.Underlying, opt);
        //                }
        //                opt.Add(inst);
        //                if (hedgeUnderlyingOptions.TryGetValue(inst.HedgeUnderlying, out opt) == false)
        //                {
        //                    opt = new List<Proto.Instrument>();
        //                    hedgeUnderlyingOptions.Add(inst.HedgeUnderlying, opt);
        //                }
        //                opt.Add(inst);
        //            }
        //        }
        //    }
        //}

        public void OnProtoMessage(Proto.InstrumentReq req)
        {
            if (req.Type == Proto.RequestType.Set)
            {
                foreach (var inst in req.Instruments)
                {
                    var instrument = FindId(inst.Id);
                    if (instrument != null)
                    {
                        instrument.Status = inst.Status;
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

        public Instrument FindId(string id)
        {
            Instrument inst = null;
            //lock (this.mutex)
            {
                instruments.TryGetValue(id, out inst);
            }
            return inst;
        }

        public IEnumerable<Instrument> GetUnderlyings()
        {
            //lock (this.mutex)
            {
                return underlyingOptions.Keys.ToList();
            }
        }

        public List<Instrument> GetUnderlyings(Instrument hedgeUnderlying)
        {
            var ret = new List<Instrument>();
            foreach (var kvp in instruments)
            {
                if (kvp.Value.Type != Proto.InstrumentType.Option && kvp.Value.HedgeUnderlying == hedgeUnderlying)
                {
                    ret.Add(kvp.Value);
                }
            }
            return ret;
        }

        public IEnumerable<Instrument> GetHedgeUnderlyings()
        {
            //lock (this.mutex)
            {
                return hedgeUnderlyingOptions.Keys;
            }
        }

        public List<Option> GetOptions(Instrument underlying)
        {
            //lock (this.mutex)
            {
                List<Option> ret = null;
                underlyingOptions.TryGetValue(underlying, out ret);
                return ret;
            }
        }

        public List<Option> GetOptionsByHedgeUnderlying(Instrument hedgeUnderlying)
        {
            //lock (this.mutex)
            {
                List<Option> ret = null;
                hedgeUnderlyingOptions.TryGetValue(hedgeUnderlying, out ret);
                return ret;
            }
        }

        public IEnumerable<Option> GetOptionsByHedgeUnderlying(Instrument hedgeUnderlying, DateTime maturity)
        {
            List<Option> options = GetOptionsByHedgeUnderlying(hedgeUnderlying);
            if (options != null)
            {
                return from option in options where option.Maturity == maturity orderby option.Id select option;
            }
            return null;
        }

        //object mutex = new object();
        Dictionary<string, Instrument> instruments = new Dictionary<string, Instrument>();
        Dictionary<Instrument, List<Option>> underlyingOptions = new Dictionary<Instrument, List<Option>>();
        Dictionary<Instrument, List<Option>> hedgeUnderlyingOptions = new Dictionary<Instrument, List<Option>>();
    }
}
