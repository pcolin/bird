using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace client.Models
{
    public class Instrument
    {
        //public Instrument(Proto.Instrument inst)
        //{
        //    Id = inst.Id;
        //    Symbol = inst.Symbol;
        //    Exchange = inst.Exchange;
        //    Type = inst.Type;
        //    Currency = inst.Currency;
        //    Tick = inst.Tick;
        //    Multiplier = inst.Multiplier;
        //    Highest = inst.Highest;
        //    Lowest = inst.Lowest;
        //    try
        //    {
        //        Maturity = DateTime.ParseExact(inst.Maturity, "yyyyMMdd", CultureInfo.InvariantCulture);
        //    }
        //    catch { }
        //}

        public string Id { get; set; }
        public string Symbol { get; set; }
        public Proto.Exchange Exchange { get; set; }
        public Proto.InstrumentType Type { get; set; }

        private object statusMutex = new object();
        private Proto.InstrumentStatus status;
        public Proto.InstrumentStatus Status
        {
            get
            {
                lock (this.statusMutex)
                {
                    return status;
                }
            }
            set
            {
                lock (this.statusMutex)
                {
                    status = value;
                }
            }
        }

        public Proto.Currency Currency { get; set; }
        public double Tick { get; set; }
        public double Multiplier { get; set; }

        private object priceMutex = new object();
        private double highest;
        public double Highest
        {
            get
            {
                lock (this.priceMutex)
                {
                    return highest;
                }
            }
            set
            {
                lock (this.priceMutex)
                {
                    highest = value;
                }
            }
        }

        private double lowest;
        public double Lowest
        {
            get
            {
                lock (this.priceMutex)
                {
                    return lowest;
                }
            }
            set
            {
                lock (this.priceMutex)
                {
                    lowest = value;
                }
            }
        }

        public DateTime Maturity { get; set; }

        public Instrument Underlying { get; set; }
        public Instrument HedgeUnderlying { get; set; }
        public string Product { get; set; }
    }
}
