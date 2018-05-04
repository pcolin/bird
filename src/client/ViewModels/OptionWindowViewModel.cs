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

        private readonly ObservableCollection<OptionUserControlViewModel> tabItems = new ObservableCollection<OptionUserControlViewModel>();
        public ObservableCollection<OptionUserControlViewModel> TabItems
        {
            get { return tabItems; }
        }
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
            this.dispatcher = dispatcher;
            this.exchange = exchange;
        }

        public void Start(Dictionary<string, OptionUserControlViewModel> viewModels)
        {
            this.viewModels = viewModels;

            var service = this.container.Resolve<ProxyService>(exchange.ToString());
            service.RegisterAction("Price", new Action<IMessage>(p => this.ReceivePrice(p)));
            service.RegisterAction("Cash", new Action<IMessage>(c => this.ReceiveCash(c)));
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

        private void ReceivePrice(IMessage msg)
        {
            var p = msg as Proto.Price;
            if (p != null)
            {
                ProductManager manager = this.container.Resolve<ProductManager>();
                var inst = manager.FindId(p.Instrument);
                OptionUserControlViewModel vm = null;
                if (inst != null && this.viewModels.TryGetValue(inst.HedgeUnderlying, out vm))
                {
                    //this.dispatcher.BeginInvoke(new OptionUserControlViewModel.ReceivePriceDelegate(vm.ReceivePrice), inst, p);
                    this.dispatcher.BeginInvoke((MethodInvoker)delegate { vm.ReceivePrice(inst, p); });
                }
            }
        }

        private void ReceiveCash(IMessage msg)
        {
            var cash = msg as Proto.Cash;
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

        private IUnityContainer container;
        private Dispatcher dispatcher;
        private Proto.Exchange exchange;
        private Dictionary<string, OptionUserControlViewModel> viewModels;
    }
}
