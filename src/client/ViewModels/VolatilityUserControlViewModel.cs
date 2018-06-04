﻿using client.Models;
using Google.Protobuf;
using Microsoft.Practices.Unity;
using Prism.Commands;
using Prism.Events;
using Prism.Mvvm;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Globalization;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Windows.Input;
using System.Windows.Threading;

namespace client.ViewModels
{
    class VolatilityUserControlViewModel : BindableBase
    {
        public ICommand ModifyCommand { get; set; }
        public ICommand RefreshCommand { get; set; }

        private Proto.Exchange exchange;
        public Proto.Exchange Exchange
        {
            get { return exchange; }
            set { SetProperty(ref exchange, value); }
        }

        private int selectedVolatility;
        public int SelectedVolatility
        {
            get { return selectedVolatility; }
            set { SetProperty(ref selectedVolatility, value); }
        }

        private ObservableCollection<VolatilityItem> volatilities;
        public ObservableCollection<VolatilityItem> Volatilities
        {
            get { return volatilities; }
            set { SetProperty(ref volatilities, value); }
        }

        public IUnityContainer Container { get; set; }

        Action<Proto.Price> PriceAction { get; set; }
        Action<Proto.SSRateReq> SSRateReqAction { get; set; }
        ProductManager productManager;
        Dictionary<Instrument, Dictionary<DateTime, VolatilityItem>> items = new Dictionary<Instrument,Dictionary<DateTime,VolatilityItem>>();
        
        public VolatilityUserControlViewModel(IUnityContainer container, Dispatcher dispatcher, Proto.Exchange exchange)
        {
            this.Container = container;
            Exchange = exchange;

            RefreshCommand = new DelegateCommand(this.Refresh);
            ModifyCommand = new DelegateCommand(this.ModifyExecute);

            this.PriceAction = p =>
                {
                    var inst = this.productManager.FindId(p.Instrument);
                    if (inst != null && inst.Type != Proto.InstrumentType.Option)
                    {
                        Dictionary<DateTime, VolatilityItem> item = null;
                        if (this.items.TryGetValue(inst, out item))
                        {
                            dispatcher.BeginInvoke((MethodInvoker)delegate
                            {
                                foreach (var kvp in item)
                                {
                                    kvp.Value.Spot = p.AdjustedPrice;
                                }
                            });
                        }
                    }
                };

            this.SSRateReqAction = req =>
                {
                    foreach (var r in req.Rates)
                    {
                        var inst = this.productManager.FindId(r.Underlying);
                        if (inst != null)
                        {
                            Dictionary<DateTime, VolatilityItem> item = null;
                            if (this.items.TryGetValue(inst, out item))
                            {
                                VolatilityItem vol = null;
                                try
                                {
                                    var maturity = DateTime.ParseExact(r.Maturity, "yyyyMMdd", CultureInfo.InvariantCulture);
                                    if (item.TryGetValue(maturity, out vol))
                                    {
                                        dispatcher.BeginInvoke((MethodInvoker)delegate { vol.SSRate = r.Rate; });
                                    }
                                }
                                catch (Exception) { }
                            }
                        }
                    }
                };
        }

        public void Initialize()
        {
            productManager = this.Container.Resolve<ProductManager>(Exchange.ToString());
            if (productManager != null)
            {
                var ssm = this.Container.Resolve<SSRateManager>(Exchange.ToString());
                VolatilityCurveManager vcm = this.Container.Resolve<VolatilityCurveManager>(Exchange.ToString());
                var volatilities = new ObservableCollection<VolatilityItem>();
                var underlyings = productManager.GetHedgeUnderlyings();
                var sortedUnderlyings = from underlying in underlyings orderby underlying.Id select underlying;
                foreach (var underlying in sortedUnderlyings)
                {
                    var maturities = new SortedSet<DateTime>();
                    var options = productManager.GetOptionsByHedgeUnderlying(underlying);
                    foreach (var option in options)
                    {
                        maturities.Add(option.Maturity);
                    }
                    foreach (var m in maturities)
                    {
                        double ssr = 0;
                        if (ssm != null)
                        {
                            var tmp = ssm.GetSSRate(underlying.Id, m);
                            if (tmp.HasValue) ssr = tmp.Value;
                        }
                        if (vcm != null)
                        {
                            var curve = vcm.GetVolatilityCurve(underlying.Id, m);
                            if (curve != null)
                            {
                                var vol = new VolatilityItem(underlying, curve, ssr);
                                volatilities.Add(vol);
                                AddVolatilityItem(vol);
                                continue;
                            }
                        }
                        var vi = new VolatilityItem(underlying, m, ssr);
                        volatilities.Add(vi);
                        AddVolatilityItem(vi);
                    }
                }
                Volatilities = volatilities;

                this.Container.Resolve<EventAggregator>().GetEvent<PubSubEvent<Proto.Price>>().Subscribe(this.PriceAction, ThreadOption.BackgroundThread);
                this.Container.Resolve<EventAggregator>().GetEvent<PubSubEvent<Proto.SSRateReq>>().Subscribe(this.SSRateReqAction, ThreadOption.BackgroundThread);
            }
        }

        private void AddVolatilityItem(VolatilityItem item)
        {
            Dictionary<DateTime, VolatilityItem> tmp = null;
            if (this.items.TryGetValue(item.Underlying, out tmp) == false)
            {
                tmp = new Dictionary<DateTime, VolatilityItem>();
                this.items.Add(item.Underlying, tmp);
            }
            tmp[item.Maturity] = item;
        }

        private void Refresh()
        {
            var vm = this.Container.Resolve<VolatilityCurveManager>(Exchange.ToString());
            if (vm == null) return;
            foreach (var kvp in this.items)
            {
                foreach (var item in kvp.Value)
                {
                    var c = vm.GetVolatilityCurve(item.Value.Underlying.Id, item.Key);
                    if (c == null)
                    {
                        c = new Proto.VolatilityCurve();
                    }
                    item.Value.Update(c);
                }
            }
        }

        private void ModifyExecute()
        {
            var req = new Proto.VolatilityCurveReq();
            foreach (var item in this.Volatilities)
            {
                if (item.Modified)
                {
                    var curve = new Proto.VolatilityCurve();
                    curve.Underlying = item.Underlying.Id;
                    curve.Maturity = item.Maturity.ToString("yyyyMMdd");
                    curve.Spot = item.SpotRef;
                    curve.AtmVol = item.AtmVol;
                    curve.Skew = item.Skew;
                    curve.CallConvex = item.CallConvex;
                    curve.PutConvex = item.PutConvex;
                    curve.CallSlope = item.CallSlope;
                    curve.PutSlope = item.PutSlope;
                    curve.Vcr = item.VCR;
                    curve.Scr = item.SCR;
                    curve.Ccr = item.CCR;
                    curve.Spcr = item.SPCR;
                    curve.Sccr = item.SCCR;

                    req.Curves.Add(curve);

                    item.Modified = false;
                }
            }

            if (req.Curves.Count > 0)
            {
                req.Type = Proto.RequestType.Set;
                var service = this.Container.Resolve<ServerService>(this.Exchange.ToString());
                service.Request(req);
            }
        }
    }

    class VolatilityItem : BindableBase
    {
        private Instrument underlying;
        public Instrument Underlying
        {
            get { return underlying; }
            private set { underlying = value; }
        }

        private DateTime maturity;
        public DateTime Maturity
        {
            get { return maturity; }
            private set { maturity = value; }
        }
        
        private bool modified;
        public bool Modified
        {
            get { return modified; }
            set { SetProperty(ref modified, value); }
        }        

        private double spot;
        public double Spot
        {
            get { return spot; }
            set { SetProperty(ref spot, value); }
        }

        private double ssrate;
        public double SSRate
        {
            get { return ssrate; }
            set { SetProperty(ref ssrate, value); }
        }        

        private double spotRef;
        public double SpotRef
        {
            get { return spotRef; }
            set { Modified = SetProperty(ref spotRef, value); }
        }

        private double atmVol;
        public double AtmVol
        {
            get { return atmVol; }
            set { Modified = SetProperty(ref atmVol, value); }
        }

        private double skew;
        public double Skew
        {
            get { return skew; }
            set { Modified = SetProperty(ref skew, value); }
        }

        private double callConvex;
        public double CallConvex
        {
            get { return callConvex; }
            set { Modified = SetProperty(ref callConvex, value); }
        }

        private double putConvex;
        public double PutConvex
        {
            get { return putConvex; }
            set { Modified = SetProperty(ref putConvex, value); }
        }

        private double callSlope;
        public double CallSlope
        {
            get { return callSlope; }
            set { Modified = SetProperty(ref callSlope, value); }
        }

        private double putSlope;
        public double PutSlope
        {
            get { return putSlope; }
            set { Modified = SetProperty(ref putSlope, value); }
        }

        private double callCutoff;
        public double CallCutoff
        {
            get { return callCutoff; }
            set { Modified = SetProperty(ref callCutoff, value); }
        }

        private double putCutoff;
        public double PutCutoff
        {
            get { return putCutoff; }
            set { Modified = SetProperty(ref putCutoff, value); }
        }

        private double vcr;
        public double VCR
        {
            get { return vcr; }
            set { Modified = SetProperty(ref vcr, value); }
        }

        private double scr;
        public double SCR
        {
            get { return scr; }
            set { Modified = SetProperty(ref scr, value); }
        }

        private double ccr;
        public double CCR
        {
            get { return ccr; }
            set { Modified = SetProperty(ref ccr, value); }
        }

        private double spcr;
        public double SPCR
        {
            get { return spcr; }
            set { Modified = SetProperty(ref spcr, value); }
        }

        private double sccr;
        public double SCCR
        {
            get { return sccr; }
            set { Modified = SetProperty(ref sccr, value); }
        }
        
        public VolatilityItem(Instrument underlying, DateTime maturity, double ssr)
        {
            this.Underlying = underlying;
            this.Maturity = maturity;
            this.ssrate = ssr;
        }

        public VolatilityItem(Instrument underlying, Proto.VolatilityCurve curve, double ssr)
        {
            this.underlying = underlying;
            this.maturity = DateTime.ParseExact(curve.Maturity, "yyyyMMdd", CultureInfo.InvariantCulture);
            this.ssrate = ssr;
            this.spotRef = curve.Spot;
            this.atmVol = curve.AtmVol;
            this.skew = curve.Skew;
            this.callConvex = curve.CallConvex;
            this.putConvex = curve.PutConvex;
            this.callSlope = curve.CallSlope;
            this.putSlope = curve.PutSlope;
            this.callCutoff = curve.CallCutoff;
            this.putCutoff = curve.PutCutoff;
            this.vcr = curve.Vcr;
            this.scr = curve.Scr;
            this.ccr = curve.Ccr;
            this.spcr = curve.Spcr;
            this.sccr = curve.Sccr;
        }

        public void Update(Proto.VolatilityCurve curve)
        {
            this.SpotRef = curve.Spot;
            this.AtmVol = curve.AtmVol;
            this.Skew = curve.Skew;
            this.CallConvex = curve.CallConvex;
            this.PutConvex = curve.PutConvex;
            this.CallSlope = curve.CallSlope;
            this.PutSlope = curve.PutSlope;
            this.CallCutoff = curve.CallCutoff;
            this.PutCutoff = curve.PutCutoff;
            this.VCR = curve.Vcr;
            this.SCR = curve.Scr;
            this.CCR = curve.Ccr;
            this.SPCR = curve.Spcr;
            this.SCCR = curve.Sccr;

            this.Modified = false;
        }
     }
}