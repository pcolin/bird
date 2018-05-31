using Google.Protobuf;
using Microsoft.Practices.Unity;
using System;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

using ModelLibrary;
using Prism.Events;
using System.Globalization;

namespace client.Models
{
    class TheoCalculator
    {
        public TheoCalculator(IUnityContainer container, Proto.Exchange exchange)
        {
            this.container = container;
            this.exchange = exchange;
        }

        public bool Start()
        {
            running = true;
            thread = new Thread(this.Work);
            thread.Start();
            return true;
        }

        public void Stop()
        {
            running = false;
            if (thread != null)
            {
                messages.CompleteAdding();
                thread.Abort();
                thread.Join();
                thread = null;
            }
        }

        public Proto.Price GetPrice(Instrument inst)
        {
            Proto.Price p = null;
            this.prices.TryGetValue(inst, out p);
            return p;
        }

        public GreeksData GetGreeks(Option option)
        {
            GreeksData data = null;
            this.greeks.TryGetValue(option, out data);
            return data;
        }

        public ImpliedVolatilityData GetIV(Option option)
        {
            ImpliedVolatilityData data = null;
            if (this.impliedVolatilities.TryGetValue(option, out data))
            {
                return data;
            }
            return null;
        }

        private void Work()
        {
            try
            {
                pm = this.container.Resolve<ProductManager>(exchange.ToString());
                epm = this.container.Resolve<ExchangeParameterManager>(exchange.ToString());
                InitializeParameters();
                RegisterMessages();
                while (running)
                {
                    IMessage msg = messages.Take();
                    Action<IMessage> action = null;
                    if (actions.TryGetValue(msg.GetType(), out action))
                    {
                        action(msg);
                    }
                }
            }
            catch (InvalidOperationException)
            {
                return;
            }
            catch (ThreadAbortException)
            {
                return;
            }
        }

        private void RegisterMessages()
        {
            actions = new Dictionary<Type, Action<IMessage>>() {
                { typeof(Proto.Price), msg => OnPrice(msg as Proto.Price) },
                { typeof(Proto.PricingSpecReq), msg => OnPricingSpecReq(msg as Proto.PricingSpecReq) },
                { typeof(Proto.ExchangeParameterReq), msg => OnExchangeParameterReq(msg as Proto.ExchangeParameterReq) },
                { typeof(Proto.InterestRateReq), msg => OnInterestRateReq(msg as Proto.InterestRateReq) },
                { typeof(Proto.SSRateReq), msg => OnSSRateReq(msg as Proto.SSRateReq) },
                { typeof(Proto.VolatilityCurveReq), msg => OnVolatilityCurveReq(msg as Proto.VolatilityCurveReq) },
            };

            this.container.Resolve<EventAggregator>().GetEvent<PubSubEvent<Proto.Price>>().Subscribe(new Action<IMessage>(msg => messages.Add(msg)));
            this.container.Resolve<EventAggregator>().GetEvent<PubSubEvent<Proto.PricingSpecReq>>().Subscribe(new Action<IMessage>(msg => messages.Add(msg)));
            this.container.Resolve<EventAggregator>().GetEvent<PubSubEvent<Proto.ExchangeParameterReq>>().Subscribe(new Action<IMessage>(msg => messages.Add(msg)));
            this.container.Resolve<EventAggregator>().GetEvent<PubSubEvent<Proto.InterestRateReq>>().Subscribe(new Action<IMessage>(msg => messages.Add(msg)));
            this.container.Resolve<EventAggregator>().GetEvent<PubSubEvent<Proto.SSRateReq>>().Subscribe(new Action<IMessage>(msg => messages.Add(msg)));
            this.container.Resolve<EventAggregator>().GetEvent<PubSubEvent<Proto.VolatilityCurveReq>>().Subscribe(new Action<IMessage>(msg => messages.Add(msg)));
        }

        private void InitializeParameters()
        {
            var psm = this.container.Resolve<PricingSpecManager>(this.exchange.ToString());
            if (psm == null) return;

            rm = this.container.Resolve<InterestRateManager>(this.exchange.ToString());
            if (rm == null) return;

            ssm = this.container.Resolve<SSRateManager>(this.exchange.ToString());
            if (ssm == null) return;

            vcm = this.container.Resolve<VolatilityCurveManager>(this.exchange.ToString());
            if (vcm == null) return;

            this.pricings = new Dictionary<Instrument, Pricing>();
            var hedgeUnderlyings = pm.GetHedgeUnderlyings();
            foreach (var underlying in hedgeUnderlyings)
            {
                var pricing = new Pricing();
                var options = pm.GetOptionsByHedgeUnderlying(underlying);
                var ps = psm.GetPricingSpec(underlying.Id);
                if (ps != null)
                {
                    pricing.PricingModel = new PricingModelWrapper(ps.Model == Proto.PricingModel.Bsm);
                    pricing.VolatilityModel = new VolatilityModelWrapper();

                    Dictionary<Option, PricingParameter> parameters = new Dictionary<Option, PricingParameter>();
                    foreach (var op in ps.Options)
                    {
                        var option = pm.FindId(op) as Option;
                        if (option != null)
                        {
                            var parameter = new PricingParameter();
                            parameter.Rate = rm.GetInterestRate(option.Maturity);
                            parameter.SSRate = ssm.GetSSRate(option.HedgeUnderlying.Id, option.Maturity);
                            var vc = vcm.GetVolatilityCurve(option.HedgeUnderlying.Id, option.Maturity);
                            if (vc != null)
                            {
                                parameter.VolatilityParameter = new VolatilityParameterWrapper()
                                    {
                                        spot = vc.Spot,
                                        skew = vc.Skew,
                                        atm_vol = vc.AtmVol,
                                        call_convex = vc.CallConvex,
                                        put_convex = vc.PutConvex,
                                        call_slope = vc.CallSlope,
                                        put_slope = vc.PutSlope,
                                        call_cutoff = vc.CallCutoff,
                                        put_cutoff = vc.PutCutoff,
                                        vcr = vc.Vcr,
                                        scr = vc.Scr,
                                        ccr = vc.Ccr,
                                        spcr = vc.Spcr,
                                        sccr = vc.Sccr
                                    };
                            }
                            parameters[option] = parameter;
                        }
                    }
                    pricing.Parameters = parameters;
                }
                this.pricings[underlying] = pricing;
            }
        }

        private void OnPrice(Proto.Price p)
        {
            var inst = pm.FindId(p.Instrument);
            if (inst != null)
            {
                if (inst.Type == Proto.InstrumentType.Option)
                {
                    /// calculate IV
                    Pricing pricing = null;
                    if (this.pricings.TryGetValue(inst.HedgeUnderlying, out pricing))
                    {
                        Option option = inst as Option;
                        PricingParameter parameter = null;
                        if (pricing.Parameters.TryGetValue(option, out parameter))
                        {
                            double? t = this.epm.GetTimeValue(option.Maturity);
                            if (t.HasValue)
                            {
                                CalculatePublishIV(option, p, pricing, parameter, t.Value);
                            }
                        }
                    }
                }
                else
                {
                    /// calculate theo
                    Pricing pricing = null;
                    if (this.pricings.TryGetValue(inst, out pricing))
                    {
                        pricing.Spot = p.AdjustedPrice;

                        Dictionary<DateTime, double?> time_values = new Dictionary<DateTime, double?>();
                        foreach (var kvp in pricing.Parameters)
                        {
                            double? tv = null;
                            if (time_values.TryGetValue(kvp.Key.Maturity, out tv) == false)
                            {
                                 tv = epm.GetTimeValue(kvp.Key.Maturity);
                            }
                            if (tv.HasValue)
                            {
                                CalculatePublishGreeks(kvp.Key, pricing, kvp.Value, tv.Value);
                            }
                        }
                    }
                    this.prices.AddOrUpdate(inst, p, (x, y) => p);
                }
            }
        }

        private void OnPricingSpecReq(Proto.PricingSpecReq req)
        {
            if (req.Exchange == this.exchange && req.Type == Proto.RequestType.Set)
            {
                foreach (var ps in req.Pricings)
                {
                    var underlying = pm.FindId(ps.Underlying);
                    if (underlying != null)
                    {
                        Pricing pricing = null;
                        if (this.pricings.TryGetValue(underlying, out pricing))
                        {
                            pricing.PricingModel = new PricingModelWrapper(ps.Model == Proto.PricingModel.Bsm);
                            var options = new SortedSet<Option>();
                            foreach (var op in ps.Options)
                            {
                                var option = pm.FindId(op) as Option;
                                if (option != null)
                                {
                                    options.Add(option);
                                    if (pricing.Parameters.ContainsKey(option) == false)
                                    {
                                        var parameter = new PricingParameter();
                                        parameter.Rate = rm.GetInterestRate(option.Maturity);
                                        parameter.SSRate = ssm.GetSSRate(option.HedgeUnderlying.Id, option.Maturity);
                                        var vc = vcm.GetVolatilityCurve(option.HedgeUnderlying.Id, option.Maturity);
                                        if (vc != null)
                                        {
                                            parameter.VolatilityParameter = new VolatilityParameterWrapper()
                                            {
                                                spot = vc.Spot,
                                                skew = vc.Skew,
                                                atm_vol = vc.AtmVol,
                                                call_convex = vc.CallConvex,
                                                put_convex = vc.PutConvex,
                                                call_slope = vc.CallSlope,
                                                put_slope = vc.PutSlope,
                                                call_cutoff = vc.CallCutoff,
                                                put_cutoff = vc.PutCutoff,
                                                vcr = vc.Vcr,
                                                scr = vc.Scr,
                                                ccr = vc.Ccr,
                                                spcr = vc.Spcr,
                                                sccr = vc.Sccr
                                            };
                                        }
                                        pricing.Parameters[option] = parameter;
                                    }
                                }
                            }
                            var itemsToRemove = pricing.Parameters.Where(p => options.Contains(p.Key) == false);
                            foreach (var kvp in itemsToRemove)
                            {
                                pricing.Parameters.Remove(kvp.Key);
                            }
                        }
                    }
                }
            }
        }

        private void OnExchangeParameterReq(Proto.ExchangeParameterReq req)
        {
            if (req.Exchange == this.exchange && req.Type == Proto.RequestType.Set)
            {
                foreach (var pricing in this.pricings)
                {
                    Dictionary<DateTime, double?> time_values = new Dictionary<DateTime, double?>();
                    foreach (var kvp in pricing.Value.Parameters)
                    {
                        double? tv = null;
                        if (time_values.TryGetValue(kvp.Key.Maturity, out tv) == false)
                        {
                            tv = epm.GetTimeValue(kvp.Key.Maturity);
                        }
                        if (tv.HasValue)
                        {
                            CalculatePublishGreeks(kvp.Key, pricing.Value, kvp.Value, tv.Value);

                            Proto.Price p = null;
                            if (this.prices.TryGetValue(kvp.Key, out p))
                            {
                                CalculatePublishIV(kvp.Key, p, pricing.Value, kvp.Value, tv.Value);
                            }
                        }
                    }
                }
            }
        }

        private void OnInterestRateReq(Proto.InterestRateReq req)
        {
            if (req.Exchange == this.exchange && req.Type == Proto.RequestType.Set)
            {
                foreach (var pricing in this.pricings)
                {
                    Dictionary<DateTime, double?> time_values = new Dictionary<DateTime, double?>();
                    foreach (var kvp in pricing.Value.Parameters)
                    {
                        kvp.Value.Rate = rm.GetInterestRate(kvp.Key.Maturity);

                        double? tv = null;
                        if (time_values.TryGetValue(kvp.Key.Maturity, out tv) == false)
                        {
                            tv = epm.GetTimeValue(kvp.Key.Maturity);
                        }
                        if (tv.HasValue)
                        {
                            CalculatePublishGreeks(kvp.Key, pricing.Value, kvp.Value, tv.Value);

                            Proto.Price p = null;
                            if (this.prices.TryGetValue(kvp.Key, out p))
                            {
                                CalculatePublishIV(kvp.Key, p, pricing.Value, kvp.Value, tv.Value);
                            }
                        }
                    }
                }
            }
        }

        private void OnSSRateReq(Proto.SSRateReq req)
        {
            if (req.Exchange == this.exchange && req.Type == Proto.RequestType.Set)
            {
                foreach (var r in req.Rates)
                {
                    var underlying = pm.FindId(r.Underlying);
                    if (underlying != null)
                    {
                        Pricing pricing = null;
                        if (this.pricings.TryGetValue(underlying, out pricing))
                        {
                            try
                            {
                                Dictionary<DateTime, double?> time_values = new Dictionary<DateTime, double?>();
                                var maturity = DateTime.ParseExact(r.Maturity, "yyyyMMdd", CultureInfo.InvariantCulture);
                                foreach (var kvp in pricing.Parameters)
                                {
                                    if (kvp.Key.Maturity == maturity)
                                    {
                                        kvp.Value.SSRate = r.Rate;

                                        double? tv = null;
                                        if (time_values.TryGetValue(kvp.Key.Maturity, out tv) == false)
                                        {
                                            tv = epm.GetTimeValue(kvp.Key.Maturity);
                                        }
                                        if (tv.HasValue)
                                        {
                                            CalculatePublishGreeks(kvp.Key, pricing, kvp.Value, tv.Value);

                                            Proto.Price p = null;
                                            if (this.prices.TryGetValue(kvp.Key, out p))
                                            {
                                                CalculatePublishIV(kvp.Key, p, pricing, kvp.Value, tv.Value);
                                            }
                                        }
                                    }
                                }
                            }
                            catch (Exception) { }
                        }
                    }
                }
            }
        }

        private void OnVolatilityCurveReq(Proto.VolatilityCurveReq req)
        {
            if (req.Exchange == this.exchange && req.Type == Proto.RequestType.Set)
            {
                foreach (var curve in req.Curves)
                {
                    var underlying = pm.FindId(curve.Underlying);
                    if (underlying != null)
                    {
                        Pricing pricing = null;
                        if (this.pricings.TryGetValue(underlying, out pricing))
                        {
                            try
                            {
                                Dictionary<DateTime, double?> time_values = new Dictionary<DateTime, double?>();
                                var maturity = DateTime.ParseExact(curve.Maturity, "yyyyMMdd", CultureInfo.InvariantCulture);
                                foreach (var kvp in pricing.Parameters)
                                {
                                    if (kvp.Key.Maturity == maturity)
                                    {
                                        if (kvp.Value.VolatilityParameter != null)
                                        {
                                            kvp.Value.VolatilityParameter.spot = curve.Spot;
                                            kvp.Value.VolatilityParameter.skew = curve.Skew;
                                            kvp.Value.VolatilityParameter.atm_vol = curve.AtmVol;
                                            kvp.Value.VolatilityParameter.call_convex = curve.CallConvex;
                                            kvp.Value.VolatilityParameter.put_convex = curve.PutConvex;
                                            kvp.Value.VolatilityParameter.call_slope = curve.CallSlope;
                                            kvp.Value.VolatilityParameter.put_slope = curve.PutSlope;
                                            kvp.Value.VolatilityParameter.call_cutoff = curve.CallCutoff;
                                            kvp.Value.VolatilityParameter.put_cutoff = curve.PutCutoff;
                                            kvp.Value.VolatilityParameter.vcr = curve.Vcr;
                                            kvp.Value.VolatilityParameter.scr = curve.Scr;
                                            kvp.Value.VolatilityParameter.ccr = curve.Ccr;
                                            kvp.Value.VolatilityParameter.spcr = curve.Spcr;
                                            kvp.Value.VolatilityParameter.sccr = curve.Sccr;
                                        }
                                        else
                                        {
                                            kvp.Value.VolatilityParameter = new VolatilityParameterWrapper()
                                            {
                                                spot = curve.Spot,
                                                skew = curve.Skew,
                                                atm_vol = curve.AtmVol,
                                                call_convex = curve.CallConvex,
                                                put_convex = curve.PutConvex,
                                                call_slope = curve.CallSlope,
                                                put_slope = curve.PutSlope,
                                                call_cutoff = curve.CallCutoff,
                                                put_cutoff = curve.PutCutoff,
                                                vcr = curve.Vcr,
                                                scr = curve.Scr,
                                                ccr = curve.Ccr,
                                                spcr = curve.Spcr,
                                                sccr = curve.Sccr
                                            };
                                        }

                                        double? tv = null;
                                        if (time_values.TryGetValue(kvp.Key.Maturity, out tv) == false)
                                        {
                                            tv = epm.GetTimeValue(kvp.Key.Maturity);
                                        }
                                        if (tv.HasValue)
                                        {
                                            CalculatePublishGreeks(kvp.Key, pricing, kvp.Value, tv.Value);

                                            Proto.Price p = null;
                                            if (this.prices.TryGetValue(kvp.Key, out p))
                                            {
                                                CalculatePublishIV(kvp.Key, p, pricing, kvp.Value, tv.Value);
                                            }
                                        }
                                    }
                                }
                            }
                            catch (Exception) { }
                        }
                    }
                }
            }
        }

        private void CalculatePublishGreeks(Option option, Pricing pricing, PricingParameter param, double timeValue)
        {
            if (!pricing.Spot.HasValue) return;
            double s = pricing.Spot.Value;

            if (!param.Rate.HasValue) return;
            double r = param.Rate.Value;

            if (!param.SSRate.HasValue) return;
            double q = param.SSRate.Value;

            if (param.VolatilityParameter == null) return;

            bool future = option.HedgeUnderlying.Type == Proto.InstrumentType.Future;
            double v = pricing.VolatilityModel.Calculate(param.VolatilityParameter, future, s, option.Strike, r, q, timeValue);
            param.Volatility = v;
            GreeksDataWrapper greeks = null;
            if (future)
            {
                greeks = pricing.PricingModel.CalculateGreeks(option.OptionType == Proto.OptionType.Call, s + q, option.Strike, v, r, r, timeValue);
            }
            else
            {
                greeks = pricing.PricingModel.CalculateGreeks(option.OptionType == Proto.OptionType.Call, s, option.Strike, v, r, q, timeValue);
            }

            var data = new GreeksData() { Option = option, Greeks = greeks };
            this.greeks.AddOrUpdate(option, data, (x, y) => data);
            this.container.Resolve<EventAggregator>().GetEvent<GreeksEvent>().Publish(data);
        }

        private void CalculatePublishIV(Option option, Proto.Price price, Pricing pricing, PricingParameter param, double timeValue)
        {
            if (!pricing.Spot.HasValue) return;
            double s = pricing.Spot.Value;

            if (!param.Rate.HasValue) return;
            double r = param.Rate.Value;

            if (!param.SSRate.HasValue) return;
            double q = param.SSRate.Value;

            //double? t = this.epm.GetTimeValue(option.Maturity);
            //if (!t.HasValue) return;

            double lastIV = 0, bidIV = 0, askIV = 0;
            bool future = option.HedgeUnderlying.Type == Proto.InstrumentType.Future;
            if (future)
            {
                bool call = option.OptionType == Proto.OptionType.Call;
                if (price.Last.Price > 0)
                {
                    lastIV = pricing.PricingModel.CalculateIV(call, param.Volatility, price.Last.Price, s + q, option.Strike, r, r, timeValue);
                }
                if (price.Bids.Count > 0 && price.Bids[0].Price > 0)
                {
                    bidIV = pricing.PricingModel.CalculateIV(call, param.Volatility, price.Bids[0].Price, s + q, option.Strike, r, r, timeValue);
                }
                if (price.Asks.Count > 0 && price.Asks[0].Price > 0)
                {
                    askIV = pricing.PricingModel.CalculateIV(call, param.Volatility, price.Asks[0].Price, s + q, option.Strike, r, r, timeValue);
                }
            }
            else
            {
                bool call = option.OptionType == Proto.OptionType.Call;
                if (price.Last.Price > 0)
                {
                    lastIV = pricing.PricingModel.CalculateIV(call, param.Volatility, price.Last.Price, s, option.Strike, r, q, timeValue);
                }
                if (price.Bids.Count > 0 && price.Bids[0].Price > 0)
                {
                    bidIV = pricing.PricingModel.CalculateIV(call, param.Volatility, price.Bids[0].Price, s, option.Strike, r, q, timeValue);
                }
                if (price.Asks.Count > 0 && price.Asks[0].Price > 0)
                {
                    askIV = pricing.PricingModel.CalculateIV(call, param.Volatility, price.Asks[0].Price, s, option.Strike, r, q, timeValue);
                }
            }

            var data = new ImpliedVolatilityData() { Option = option, LastIV = lastIV, BidIV = bidIV, AskIV = askIV };
            this.impliedVolatilities.AddOrUpdate(option, data, (x, y) => data);
            this.container.Resolve<EventAggregator>().GetEvent<ImpliedVolatilityEvent>().Publish(data);
        }

        IUnityContainer container;
        Proto.Exchange exchange;

        class PricingParameter
        {
            public double? Rate { get; set; }
            public double? SSRate { get; set; }
            public double Volatility { get; set; }
            public VolatilityParameterWrapper VolatilityParameter { get; set; }
        }

        class Pricing
        {
            public PricingModelWrapper PricingModel { get; set; }
            public VolatilityModelWrapper VolatilityModel { get; set; }

            public double? Spot { get; set; }
            public Dictionary<Option, PricingParameter> Parameters { get; set; }
        }

        ConcurrentDictionary<Instrument, Proto.Price> prices = new ConcurrentDictionary<Instrument,Proto.Price>();
        ConcurrentDictionary<Option, GreeksData> greeks = new ConcurrentDictionary<Option, GreeksData>();
        ConcurrentDictionary<Option, ImpliedVolatilityData> impliedVolatilities = new ConcurrentDictionary<Option, ImpliedVolatilityData>();

        Dictionary<Instrument, Pricing> pricings;
        Dictionary<Type, Action<IMessage>> actions;

        private BlockingCollection<IMessage> messages = new BlockingCollection<IMessage>();
        private volatile bool running = false;
        private Thread thread;

        private ProductManager pm = null;
        private ExchangeParameterManager epm = null;
        private InterestRateManager rm = null;
        private SSRateManager ssm = null;
        private VolatilityCurveManager vcm = null;
    }
}
