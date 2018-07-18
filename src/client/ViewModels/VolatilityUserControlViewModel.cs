using client.Models;
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
    public class VolatilityUserControlViewModel : BindableBase
    {
        public ICommand SetSpotRefCommand { get; set; }
        public ICommand SetSsrCommand { get; set; }
        public ICommand SetVolCommand { get; set; }
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
        private Dispatcher dispatcher;

        //Action<Proto.Price> PriceAction { get; set; }
        //Action<Proto.SSRateReq> SSRateReqAction { get; set; }
        ProductManager productManager;
        Dictionary<Instrument, Dictionary<DateTime, VolatilityItem>> items = new Dictionary<Instrument,Dictionary<DateTime,VolatilityItem>>();
        
        public VolatilityUserControlViewModel(IUnityContainer container, Dispatcher dispatcher, Proto.Exchange exchange)
        {
            this.Container = container;
            this.dispatcher = dispatcher;
            Exchange = exchange;

            productManager = this.Container.Resolve<ProductManager>(Exchange.ToString());
            if (productManager != null)
            {
                RefreshCommand = new DelegateCommand(this.Refresh);
                SetSpotRefCommand = new DelegateCommand(this.SetSpotRefExecute);
                SetSsrCommand = new DelegateCommand(this.SetSsrExecute, this.CanSetSsrExecute);
                SetVolCommand = new DelegateCommand(this.SetVolExecute, this.CanSetVolExecute);

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
                            ssr = ssm.GetSSRate(underlying.Id, m);
                        }
                        if (vcm != null)
                        {
                            var curve = vcm.GetVolatilityCurve(underlying.Id, m);
                            if (curve != null)
                            {
                                var vol = new VolatilityItem(underlying, curve, ssr, this.SetVolCommand, this.SetSsrCommand);
                                volatilities.Add(vol);
                                AddVolatilityItem(vol);
                                continue;
                            }
                        }
                        var vi = new VolatilityItem(underlying, m, ssr, this.SetVolCommand, this.SetSsrCommand);
                        volatilities.Add(vi);
                        AddVolatilityItem(vi);
                    }
                }
                Volatilities = volatilities;
            }
        }

        public void ReceivePrice(Proto.Price p)
        {
            var inst = this.productManager.FindId(p.Instrument);
            if (inst != null && inst.Type != Proto.InstrumentType.Option)
            {
                Dictionary<DateTime, VolatilityItem> item = null;
                if (this.items.TryGetValue(inst, out item))
                {
                    dispatcher.Invoke((MethodInvoker)delegate
                    {
                        foreach (var kvp in item)
                        {
                            kvp.Value.Spot = p.AdjustedPrice;
                        }
                    });
                }
            }
        }

        public void ReceiveSSRateReq(Proto.SSRateReq req)
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
                                dispatcher.Invoke((MethodInvoker)delegate
                                {
                                    vol.SSRate = r.Rate;
                                    vol.SSRateModified = false;
                                });
                            }
                        }
                        catch (Exception) { }
                    }
                }
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

        private void SetSpotRefExecute()
        {
            var item = this.Volatilities.ElementAt(this.SelectedVolatility);
            if (item != null && double.IsNaN(item.Spot) == false && double.IsNaN(item.SSRate) == false)
            {
                item.SpotRef = item.Spot + item.SSRate;
            }
        }

        private void SetSsrExecute()
        {
            var req = new Proto.SSRateReq();
            req.Type = Proto.RequestType.Set;
            req.Exchange = this.Exchange;
            foreach (var item in this.Volatilities)
            {
                if (item.SSRateModified)
                {
                    var ssr = new Proto.SSRate();
                    ssr.Underlying = item.Underlying.Id;
                    ssr.Maturity = item.Maturity.ToString("yyyyMMdd");
                    ssr.Rate = item.SSRate;

                    req.Rates.Add(ssr);
                    item.SSRateModified = false;
                }
            }

            var service = this.Container.Resolve<ServerService>(this.Exchange.ToString());
            req.User = service.User;
            service.Request(req);
        }

        private bool CanSetSsrExecute()
        {
            foreach (var item in this.Volatilities)
            {
                if (item.SSRateModified) return true;
            }
            return false;
        }

        private void SetVolExecute()
        {
            var req = new Proto.VolatilityCurveReq() { Type = Proto.RequestType.Set, Exchange = this.Exchange };
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
                    curve.CallCutoff = item.CallCutoff;
                    curve.PutCutoff = item.PutCutoff;
                    curve.Vcr = item.VCR;
                    curve.Scr = item.SCR;
                    curve.Ccr = item.CCR;
                    curve.Spcr = item.SPCR;
                    curve.Sccr = item.SCCR;

                    req.Curves.Add(curve);

                    item.Modified = false;
                }
            }

            var service = this.Container.Resolve<ServerService>(this.Exchange.ToString());
            req.User = service.User;
            service.Request(req);
        }

        private bool CanSetVolExecute()
        {
            foreach (var item in this.Volatilities)
            {
                if (item.Modified) return true;
            }
            return false;
        }
    }

    public class VolatilityItem : BindableBase
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
            set
            {
                if (SetProperty(ref modified, value))
                {
                    (this.volCommand as DelegateCommand).RaiseCanExecuteChanged();
                }
            }
        }

        private bool ssrateModified;
        public bool SSRateModified
        {
            get { return ssrateModified; }
            set
            {
                if (SetProperty(ref ssrateModified, value))
                {
                    (this.ssrCommand as DelegateCommand).RaiseCanExecuteChanged();
                }
            }
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
            set { SSRateModified = SetProperty(ref ssrate, value); }
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

        private ICommand volCommand;
        private ICommand ssrCommand;
        
        public VolatilityItem(Instrument underlying, DateTime maturity, double ssr, ICommand volCommand, ICommand ssrCommand)
        {
            this.Underlying = underlying;
            this.Maturity = maturity;
            this.ssrate = ssr;

            this.volCommand = volCommand;
            this.ssrCommand = ssrCommand;
        }

        public VolatilityItem(Instrument underlying, Proto.VolatilityCurve curve, double ssr, ICommand volCommand, ICommand ssrCommand)
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

            this.volCommand = volCommand;
            this.ssrCommand = ssrCommand;
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
