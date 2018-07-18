using client.Models;
using Google.Protobuf;
using Microsoft.Practices.Unity;
using Prism.Events;
using Prism.Interactivity.InteractionRequest;
using Prism.Mvvm;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Forms;
using System.Windows.Threading;

namespace client.ViewModels
{
    class OptionWindowViewModel : BindableBase
    {
        public InteractionRequest<INotification> NotificationRequest { get; set; }

        //private readonly ObservableCollection<OptionUserControlViewModel> tabItems = new ObservableCollection<OptionUserControlViewModel>();
        //public ObservableCollection<OptionUserControlViewModel> TabItems
        //{
        //    get { return tabItems; }
        //}
        //private readonly List<BindableBase> tabItems = new List<BindableBase>();
        //public List<BindableBase> TabItems
        //{
        //    get { return tabItems; }
        //}

        private BindableBase selectedTab;
        public BindableBase SelectedTab
        {
            get { return selectedTab; }
            set { SetProperty(ref selectedTab, value); }
        }        

        public Proto.Exchange Exchange
        {
            get { return exchange; }
        }

        public IUnityContainer Container
        {
            get { return container; }
        }

        private string account;
        public string Account
        {
            get { return account; }
            set { SetProperty(ref account, value); }
        }        

        private double available;
        public double Available
        {
            get { return available; }
            set
            {
                if (SetProperty(ref available, value))
                {
                    if (value > maxAvailable || double.IsNaN(maxAvailable))
                    {
                        MaxAvailable = value;
                    }
                    if (value < minAvailable || double.IsNaN(minAvailable))
                    {
                        MinAvailable = value;
                    }
                }
            }
        }

        private double maxAvailable = double.NaN;
        public double MaxAvailable
        {
            get { return maxAvailable; }
            set { SetProperty(ref maxAvailable, value); }
        }

        private double minAvailable = double.NaN;
        public double MinAvailable
        {
            get { return minAvailable; }
            set { SetProperty(ref minAvailable, value); }
        }
        
        private double margin;
        public double Margin
        {
            get { return margin; }
            set
            {
                if (SetProperty(ref margin, value))
                {
                    if (value > maxMargin || double.IsNaN(maxMargin))
                    {
                        MaxMargin = value;
                    }
                    if (value < minMargin || double.IsNaN(minMargin))
                    {
                        MinMargin = value;
                    }
                }
            }
        }

        private double maxMargin = double.NaN;
        public double MaxMargin
        {
            get { return maxMargin; }
            set { SetProperty(ref maxMargin, value); }
        }

        private double minMargin = double.NaN;
        public double MinMargin
        {
            get { return minMargin; }
            set { SetProperty(ref minMargin, value); }
        }
        
        public OptionWindowViewModel(IUnityContainer container, Dispatcher dispatcher, Proto.Exchange exchange)
        {
            this.container = container;
            //this.container.Resolve<EventAggregator>().GetEvent<StartWindowEvent>().Subscribe(this.StartWindow, ThreadOption.PublisherThread);
            //this.container.Resolve<EventAggregator>().GetEvent<GreeksEvent>().Subscribe(this.ReceiveGreeks, ThreadOption.BackgroundThread);
            //this.container.Resolve<EventAggregator>().GetEvent<ImpliedVolatilityEvent>().Subscribe(this.ReceiveIV, ThreadOption.BackgroundThread);
            this.dispatcher = dispatcher;
            this.exchange = exchange;
        }

        public void Start(Dictionary<Instrument, OptionUserControlViewModel> viewModels)
        {
            this.viewModels = viewModels;

            //var service = this.container.Resolve<ProxyService>(exchange.ToString());
            //service.RegisterAction("Price", new Action<IMessage>(p => this.ReceivePrice(p)));
            //service.RegisterAction("Cash", new Action<IMessage>(c => this.ReceiveCash(c)));

            this.container.Resolve<EventAggregator>().GetEvent<GreeksEvent>().Subscribe(this.ReceiveGreeks, ThreadOption.BackgroundThread);
            this.container.Resolve<EventAggregator>().GetEvent<ImpliedVolatilityEvent>().Subscribe(this.ReceiveIV, ThreadOption.BackgroundThread);
            this.container.Resolve<EventAggregator>().GetEvent<PubSubEvent<Proto.InstrumentReq>>().Subscribe(this.ReceiveInstrumentReq, ThreadOption.BackgroundThread);
            this.container.Resolve<EventAggregator>().GetEvent<PubSubEvent<Proto.Price>>().Subscribe(this.ReceivePrice, ThreadOption.BackgroundThread);
            this.container.Resolve<EventAggregator>().GetEvent<PubSubEvent<Proto.Cash>>().Subscribe(this.ReceiveCash, ThreadOption.BackgroundThread);
            this.container.Resolve<EventAggregator>().GetEvent<PubSubEvent<Proto.DestrikerReq>>().Subscribe(this.ReceiveDestrikerReq, ThreadOption.BackgroundThread);
            this.container.Resolve<EventAggregator>().GetEvent<PubSubEvent<Proto.CreditReq>>().Subscribe(this.ReceiveCreditReq, ThreadOption.BackgroundThread);
            this.container.Resolve<EventAggregator>().GetEvent<PubSubEvent<Proto.StrategySwitchReq>>().Subscribe(this.ReceiveStrategySwitchReq, ThreadOption.BackgroundThread);

            this.pm = this.container.Resolve<ProductManager>(this.exchange.ToString());
            var ss = this.container.Resolve<ServerService>(this.exchange.ToString());
            this.user = ss.User;
        }

        //private void StartWindow()
        //{
        //    dispatcher.Invoke(() =>
        //    {
        //        ProductManager manager = this.container.Resolve<ProductManager>();
        //        List<string> hedgeUnderlyings = manager.GetHedgeUnderlyings(exchange);
        //        for (int i = 0; i < hedgeUnderlyings.Count; ++i)
        //        {
        //            var underlyings = manager.GetUnderlyings(hedgeUnderlyings[i]);
        //            var options = manager.GetOptionsByHedgeUnderlying(hedgeUnderlyings[i]);
        //            OptionUserControlViewModel vm = new OptionUserControlViewModel(hedgeUnderlyings[i], underlyings, options);
        //            //if (i == 0)
        //            //{
        //            //    tabItems[0] = vm;
        //            //}
        //            //else
        //            {
        //                tabItems.Add(vm);
        //            }
        //            this.viewModels[hedgeUnderlyings[i]] = vm;
        //        }
        //    });
        //    var service = this.container.Resolve<ProxyService>(exchange.ToString());
        //    service.RegisterAction((new Proto.Price()).GetType().ToString(), new Action<IMessage>(p => this.ReceivePrice(p)));
        //}

        private void ReceiveInstrumentReq(Proto.InstrumentReq req)
        {
            foreach (var inst in req.Instruments)
            {
                var instrument = pm.FindId(inst.Id);
                OptionUserControlViewModel vm = null;
                if (instrument != null && this.viewModels.TryGetValue(instrument.HedgeUnderlying, out vm))
                {
                    this.dispatcher.Invoke((MethodInvoker)delegate { vm.RefreshInstrumentStatus(instrument); });
                }
            }
        }

        private void ReceivePrice(Proto.Price p)
        {
            if (p != null)
            {
                var inst = pm.FindId(p.Instrument);
                OptionUserControlViewModel vm = null;
                if (inst != null && this.viewModels.TryGetValue(inst.HedgeUnderlying, out vm))
                {
                    //this.dispatcher.BeginInvoke(new OptionUserControlViewModel.ReceivePriceDelegate(vm.ReceivePrice), inst, p);
                    this.dispatcher.Invoke((MethodInvoker)delegate { vm.ReceivePrice(inst, p); });
                }
            }
        }

        private void ReceiveCash(Proto.Cash cash)
        {
            if (cash != null)
            {
                this.dispatcher.BeginInvoke((MethodInvoker)delegate
                {
                    Account = cash.Account;
                    Available = cash.Available;
                    Margin = cash.Margin;
                });
            }
        }

        private void ReceiveGreeks(GreeksData greeks)
        {
            OptionUserControlViewModel vm = null;
            if (this.viewModels.TryGetValue(greeks.Option.HedgeUnderlying, out vm))
            {
                this.dispatcher.Invoke((MethodInvoker)delegate { vm.ReceiveGreeks(greeks); });
            }
        }

        private void ReceiveIV(ImpliedVolatilityData data)
        {
            OptionUserControlViewModel vm = null;
            if (this.viewModels.TryGetValue(data.Option.HedgeUnderlying, out vm))
            {
                this.dispatcher.BeginInvoke((MethodInvoker)delegate { vm.ReceiveIV(data); });
            }
        }

        private void ReceiveDestrikerReq(Proto.DestrikerReq req)
        {
            if (req.User != this.user)
            {
                foreach (var d in req.Destrikers)
                {
                    var option = pm.FindId(d.Instrument) as Option;
                    OptionUserControlViewModel vm = null;
                    if (option != null && this.viewModels.TryGetValue(option.HedgeUnderlying, out vm))
                    {
                        //this.dispatcher.BeginInvoke((MethodInvoker)delegate { vm.RefreshDestriker(instrument, d.Destriker_); });
                        this.dispatcher.Invoke((MethodInvoker)delegate { vm.RefreshDestriker(option, d.Destriker_); });
                    }
                }
            }
        }

        private void ReceiveCreditReq(Proto.CreditReq req)
        {
            foreach (var c in req.Credits)
            {
                var underlying = pm.FindId(c.Underlying);
                OptionUserControlViewModel vm = null;
                if (underlying != null && this.viewModels.TryGetValue(underlying, out vm))
                {
                    this.dispatcher.Invoke((MethodInvoker)delegate { vm.RefreshCredit(c); });
                }
            }
        }

        private void ReceiveStrategySwitchReq(Proto.StrategySwitchReq req)
        {
            if (req.User != this.user)
            {
                foreach (var s in req.Switches)
                {
                    var option = pm.FindId(s.Option) as Option;
                    OptionUserControlViewModel vm = null;
                    if (option != null && this.viewModels.TryGetValue(option.HedgeUnderlying, out vm))
                    {
                        this.dispatcher.Invoke((MethodInvoker)delegate { vm.RefreshStrategySwitch(option, s); });
                    }
                }
            }
        }

        //private void ReceiveQuoterReq(Proto.QuoterReq req)
        //{
        //    foreach (var q in req.Quoters)
        //    {
        //        if (q.Records.Count > 0)
        //        {
        //            var instrument = pm.FindId(q.Underlying);
        //            OptionUserControlViewModel vm = null;
        //            if (instrument != null && this.viewModels.TryGetValue(instrument, out vm))
        //            {
        //                this.dispatcher.Invoke((MethodInvoker)delegate { vm.RefreshQuoterRecord(q.Records); });
        //            }                    
        //        }
        //    }
        //}

        private IUnityContainer container;
        private Dispatcher dispatcher;
        private Proto.Exchange exchange;
        private ProductManager pm;
        private string user;
        private Dictionary<Instrument, OptionUserControlViewModel> viewModels;
    }
}
