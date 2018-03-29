using client.Models;
using Dragablz;
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
using System.Windows.Threading;

namespace client.ViewModels
{
    class OptionWindowViewModel : BindableBase
    {
        public InteractionRequest<INotification> NotificationRequest { get; set; }

        private readonly ObservableCollection<BindableBase> tabItems = new ObservableCollection<BindableBase>();
        public ObservableCollection<BindableBase> TabItems
        {
            get { return tabItems; }
        }

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

        private string account = "99665550";
        public string Account
        {
            get { return account; }
            set { SetProperty(ref account, value); }
        }        

        private double available = -123456;
        public double Available
        {
            get { return available; }
            set
            {
                if (SetProperty(ref available, value))
                {
                    if (value > maxAvailable)
                    {
                        MaxAvailable = value;
                    }
                    if (value < minAvailable)
                    {
                        MinAvailable = value;
                    }
                }
            }
        }

        private double maxAvailable = -123456;
        public double MaxAvailable
        {
            get { return maxAvailable; }
            set { SetProperty(ref maxAvailable, value); }
        }

        private double minAvailable = -123456;
        public double MinAvailable
        {
            get { return minAvailable; }
            set { SetProperty(ref minAvailable, value); }
        }
        
        private double margin = 12345;
        public double Margin
        {
            get { return margin; }
            set
            {
                if (SetProperty(ref margin, value))
                {
                    if (value > maxMargin)
                    {
                        MaxMargin = value;
                    }
                    if (value < minMargin)
                    {
                        MinMargin = value;
                    }
                }
            }
        }

        private double maxMargin = 12345;
        public double MaxMargin
        {
            get { return maxMargin; }
            set { SetProperty(ref maxMargin, value); }
        }

        private double minMargin = 12345;
        public double MinMargin
        {
            get { return minMargin; }
            set { SetProperty(ref minMargin, value); }
        }
        
        public OptionWindowViewModel(IUnityContainer container, Dispatcher dispatcher, Proto.Exchange exchange)
        {
            this.container = container;
            this.container.Resolve<EventAggregator>().GetEvent<StartWindowEvent>().Subscribe(this.StartWindow, ThreadOption.PublisherThread);
            this.dispatcher = dispatcher;
            this.exchange = exchange;
        }

        private void StartWindow()
        {
            dispatcher.Invoke(() =>
            {
                ProductManager manager = this.container.Resolve<ProductManager>();
                List<string> hedgeUnderlyings = manager.GetHedgeUnderlyings(exchange);
                for (int i = 0; i < hedgeUnderlyings.Count; ++i)
                {
                    var underlyings = manager.GetUnderlyings(hedgeUnderlyings[i]);
                    var options = manager.GetOptionsByHedgeUnderlying(hedgeUnderlyings[i]);
                    OptionUserControlViewModel vm = new OptionUserControlViewModel(hedgeUnderlyings[i], underlyings, options);
                    //if (i == 0)
                    //{
                    //    tabItems[0] = vm;
                    //}
                    //else
                    {
                        tabItems.Add(vm);
                    }
                }
            });
        }

        private IUnityContainer container;
        private Dispatcher dispatcher;
        private Proto.Exchange exchange;
    }
}
