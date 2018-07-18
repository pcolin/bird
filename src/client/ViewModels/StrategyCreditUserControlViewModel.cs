using client.Models;
using Microsoft.Practices.Unity;
using Prism.Commands;
using Prism.Mvvm;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace client.ViewModels
{
    public class StrategyCreditUserControlViewModel : BindableBase
    {
        public DelegateCommand RefreshCommand { get; set; }
        public DelegateCommand SetCommand { get; set; }

        private Proto.Exchange exchange;
        public Proto.Exchange Exchange
        {
            get { return exchange; }
            set { SetProperty(ref exchange, value); }
        }

        private ObservableCollection<CreditItem> credits;
        public ObservableCollection<CreditItem> Credits
        {
            get { return credits; }
            set { SetProperty(ref credits, value); }
        } 
       
        public StrategyCreditUserControlViewModel(Proto.Exchange exchange, Proto.StrategyType strategy, IUnityContainer container)
        {
            this.exchange = exchange;
            this.strategy = strategy;
            this.container = container;

            var pm = container.Resolve<ProductManager>(exchange.ToString());
            if (pm != null)
            {
                credits = new ObservableCollection<CreditItem>();
                var cm = container.Resolve<CreditManager>(exchange.ToString());
                var underlyings = pm.GetHedgeUnderlyings();
                var sortedUnderlyings = from underlying in underlyings orderby underlying.Id select underlying;
                foreach (var underlying in sortedUnderlyings)
                {
                    var maturities = new SortedSet<DateTime>();
                    var options = pm.GetOptionsByHedgeUnderlying(underlying);
                    foreach (var option in options)
                    {
                        maturities.Add(option.Maturity);
                    }
                    foreach (var m in maturities)
                    {
                        CreditItem item = null;
                        var credit = cm.GetCredit(strategy, underlying.Id, m);
                        if (credit != null)
                        {
                            item = new CreditItem(underlying, m, credit);
                        }
                        else
                        {
                            item = new CreditItem(underlying, m);
                        }
                        credits.Add(item);
                    }
                }

                RefreshCommand = new DelegateCommand(this.RefreshExecute);
                //SetCommand = new DelegateCommand(this.SetExecute, this.CanSetExecute);
                SetCommand = new DelegateCommand(this.SetExecute);
            }
        }

        void RefreshExecute()
        { 
            var cm = container.Resolve<CreditManager>(exchange.ToString());
            if (cm == null) return;

            foreach (var item in this.Credits)
            {
                var credit = cm.GetCredit(this.strategy, item.Underlying.Id, item.Maturity);
                if (credit != null)
                {
                    item.Update(credit);
                }
            }
            //UnsetModified();
        }

        void SetExecute()
        {
            Proto.CreditReq req = new Proto.CreditReq();
            req.Type = Proto.RequestType.Set;
            req.Exchange = Exchange;

            //Proto.QuoterReq quoterReq = new Proto.QuoterReq();
            //quoterReq.Type = Proto.RequestType.Set;
            //quoterReq.Exchange = Exchange;

            var pm = this.container.Resolve<ProductManager>(Exchange.ToString());
            var tc = this.container.Resolve<TheoCalculator>(Exchange.ToString());
            var qm = this.container.Resolve<QuoterManager>(Exchange.ToString());

            foreach (var c in this.Credits)
            {
                if (c.Multiplier > 0)
                {
                    var credit = new Proto.Credit()
                        {
                            Strategy = strategy,
                            Underlying = c.Underlying.Id,
                            Maturity = c.Maturity.ToString("yyyyMMdd"),
                            Delta = c.Delta,
                            Vega = c.Vega,
                            Skew = c.Skew,
                            Convex = c.Convex,
                            Cash = c.Cash,
                            Price = c.Price,
                            Multiplier = c.Multiplier,
                        };
                    var options = pm.GetOptionsByHedgeUnderlying(c.Underlying, c.Maturity);
                    foreach (var option in options)
                    {
                        var greeks = tc.GetGreeks(option);
                        if (greeks != null)
                        {
                            double convexSensi = 0;
                            if (option.OptionType == Proto.OptionType.Call)
                            {
                                if (double.IsNaN(greeks.CallConvexSensi) == false)
                                    convexSensi = greeks.CallConvexSensi;
                            }
                            else if (double.IsNaN(greeks.PutConvexSensi) == false)
                            {
                                convexSensi = greeks.PutConvexSensi;
                            }
                            double newCredit = Math.Abs(greeks.Greeks.delta) * c.Delta + greeks.Greeks.vega * c.Vega + Math.Abs(greeks.SkewSensi) * c.Skew + Math.Abs(convexSensi) * c.Convex +
                                c.Cash + greeks.Greeks.theo * c.Price;
                            credit.Records.Add(new Proto.CreditRecord()
                                {
                                    Option = option.Id,
                                    Credit = newCredit,
                                });
                        }
                    }
                    req.Credits.Add(credit);
                    c.Modified = false;
                }

                //var quoter = qm.GetQuoter(c.Underlying.Id);
                //if (quoter != null)
                //{
                //    var q = new Proto.QuoterSpec()
                //        {
                //            Name = quoter.Name,
                //            Underlying = quoter.Underlying,
                //        };
                //    var options = pm.GetOptionsByHedgeUnderlying(c.Underlying);
                //    foreach (var option in options)
                //    {
                //        var greeks = tc.GetGreeks(option);
                //        if (greeks != null)
                //        {
                //            double convexSensi = 0;
                //            if (option.OptionType == Proto.OptionType.Call)
                //            {
                //                if (double.IsNaN(greeks.CallConvexSensi) == false)
                //                    convexSensi = greeks.CallConvexSensi;
                //            }
                //            else if (double.IsNaN(greeks.PutConvexSensi) == false)
                //            {
                //                convexSensi = greeks.PutConvexSensi;
                //            }
                //            double newCredit = Math.Abs(greeks.Greeks.delta) * c.Delta + greeks.Greeks.vega * c.Vega + Math.Abs(greeks.SkewSensi) * c.Skew + Math.Abs(convexSensi) * c.Convex +
                //                c.Cash + greeks.Greeks.theo * c.Price;
                //            if (newCredit > 0 || c.Multiplier > 0)
                //            {
                //                q.Records.Add(new Proto.QuoterRecord()
                //                    {
                //                        Instrument = option.Id,
                //                        Credit = newCredit,
                //                        Multiplier = c.Multiplier,
                //                    });
                //            }
                //        }
                //    }
                //    if (q.Records.Count > 0)
                //    {
                //        quoterReq.Quoters.Add(q);
                //    }
                //}
            }
            var service = this.container.Resolve<ServerService>(Exchange.ToString());
            req.User = service.User;
            service.Request(req);
            //UnsetModified();

            //if (quoterReq.Quoters.Count > 0)
            //{
            //    quoterReq.User = service.User;
            //    service.Request(quoterReq);
            //}
        }

        //bool CanSetExecute()
        //{
        //    return modified;
        //}

        //private void SetModified()
        //{
        //    modified = true;
        //    SetCommand.RaiseCanExecuteChanged();
        //}

        //private void UnsetModified()
        //{
        //    modified = false;
        //    SetCommand.RaiseCanExecuteChanged();
        //}

        private Proto.StrategyType strategy;
        private IUnityContainer container;
        //private bool modified = false;
    }

    public class CreditItem : BindableBase
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

        private double delta;
        public double Delta
        {
            get { return delta; }
            set
            {
                if (SetProperty(ref delta, value))
                {
                    Modified = true;
                }
            }
        }

        private double vega;
        public double Vega
        {
            get { return vega; }
            set
            {
                if (SetProperty(ref vega, value))
                {
                    Modified = true;
                }
            }
        }

        private double skew;
        public double Skew
        {
            get { return skew; }
            set
            {
                if (SetProperty(ref skew, value))
                {
                    Modified = true;
                }
            }
        }

        private double convex;
        public double Convex
        {
            get { return convex; }
            set
            {
                if (SetProperty(ref convex, value))
                {
                    Modified = true;
                }
            }
        }

        private double cash;
        public double Cash
        {
            get { return cash; }
            set
            {
                if (SetProperty(ref cash, value))
                {
                    Modified = true;
                }
            }
        }

        private double price;
        public double Price
        {
            get { return price; }
            set
            {
                if (SetProperty(ref price, value))
                {
                    Modified = true;
                }
            }
        }

        private double multiplier;
        public double Multiplier
        {
            get { return multiplier; }
            set
            {
                if (SetProperty(ref multiplier, value))
                {
                    Modified = true;
                }
            }
        }

        private bool modified;
        public bool Modified
        {
            get { return modified; }
            set { SetProperty(ref modified, value); }
        }        

        public CreditItem(Instrument underlying, DateTime maturity, Proto.Credit credit)
        {
            //this.action = action;
            this.underlying = underlying;
            this.maturity = maturity;
            this.delta = credit.Delta;
            this.vega = credit.Vega;
            this.skew = credit.Skew;
            this.convex = credit.Convex;
            this.cash = credit.Cash;
            this.price = credit.Price;
            this.multiplier = credit.Multiplier;
        } 
       
        public CreditItem(Instrument underlying, DateTime maturity)
        {
            //this.action = action;
            this.underlying = underlying;
            this.maturity = maturity;
        }

        public void Update(Proto.Credit credit)
        {
            this.delta = credit.Delta;
            this.vega = credit.Vega;
            this.skew = credit.Skew;
            this.convex = credit.Convex;
            this.cash = credit.Cash;
            this.price = credit.Price;
            this.multiplier = credit.Multiplier;
            this.Modified = false;
        }

        //private Action action;
    }
}
