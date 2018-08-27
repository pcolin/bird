using Microsoft.Practices.Unity;
using Prism.Events;
using Prism.Mvvm;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Data;
using System.Windows.Forms;
using System.Windows.Threading;

namespace client.ViewModels
{
    class MessageWindowViewModel : BindableBase
    {
        public ICollectionView MessageView { get; set; }

        private ObservableCollection<Proto.ServerInfo> messages;
        public ObservableCollection<Proto.ServerInfo> Messages
        {
            get { return messages; }
            set
            {
                if (SetProperty(ref messages, value))
                {
                    MessageView = CollectionViewSource.GetDefaultView(value);
                    MessageView.Filter = this.Filter;
                }
            }
        }

        public ICollectionView TypesView { get; set; }
        private ObservableCollection<FilterItem<Proto.InfoType>> types;
        private ObservableCollection<FilterItem<Proto.InfoType>> Types
        {
            get { return types; }
            set
            {
                if (SetProperty(ref types, value))
                {
                    TypesView = CollectionViewSource.GetDefaultView(value);
                }
            }
        }

        public ICollectionView ExchangesView { get; set; }
        private ObservableCollection<FilterItem<Proto.Exchange>> exchanges;
        private ObservableCollection<FilterItem<Proto.Exchange>> Exchanges
        {
            get { return exchanges; }
            set
            {
                if (SetProperty(ref exchanges, value))
                {
                    ExchangesView = CollectionViewSource.GetDefaultView(value);
                }
            }
        }        

        Action<Proto.ServerInfo> InfoAction { get; set; }
        IUnityContainer Container { get; set; }
        Dispatcher dispatcher;

        HashSet<Proto.Exchange> excludedExchanges = new HashSet<Proto.Exchange>();
        HashSet<Proto.InfoType> excludedTypes = new HashSet<Proto.InfoType>();

        public MessageWindowViewModel(IUnityContainer container, Dispatcher dispatcher)
        {
            InfoAction = new Action<Proto.ServerInfo>(i => dispatcher.BeginInvoke((MethodInvoker)delegate
            {
                this.Messages.Add(i);
            }));
            Messages = new ObservableCollection<Proto.ServerInfo>();
            var types = new ObservableCollection<FilterItem<Proto.InfoType>>();
            types.Add(new FilterItem<Proto.InfoType>(this.FilterType));
            Func<Proto.InfoType, string> typeFunc = t => t.ToString();
            foreach (var t in Enum.GetValues(typeof(Proto.InfoType)))
            {
                types.Add(new FilterItem<Proto.InfoType>(this.FilterType, typeFunc, (Proto.InfoType)t));
            }
            this.Types = types;

            var filterExchanges = new ObservableCollection<FilterItem<Proto.Exchange>>();
            filterExchanges.Add(new FilterItem<Proto.Exchange>(this.FilterExchange));
            this.Exchanges = filterExchanges;

            container.Resolve<EventAggregator>().GetEvent<PubSubEvent<List<Proto.Exchange>>>().Subscribe(this.StartWindow, ThreadOption.PublisherThread);
            this.Container = container;
            this.dispatcher = dispatcher;
        }

        private void StartWindow(List<Proto.Exchange> exchanges)
        {
            if (exchanges.Count > 0)
            {
                Container.Resolve<EventAggregator>().GetEvent<PubSubEvent<Proto.ServerInfo>>().Subscribe(this.InfoAction, ThreadOption.PublisherThread);

                this.dispatcher.Invoke((MethodInvoker)delegate
                    {
                        foreach (var exchange in exchanges)
                        {
                            this.Exchanges.Add(new FilterItem<Proto.Exchange>(this.FilterExchange, e => e.ToString(), exchange));
                        }
                    });
            }
        }

        private bool Filter(object item)
        {
            var it = item as Proto.ServerInfo;
            return !(excludedExchanges.Contains(it.Exchange) || excludedTypes.Contains(it.Type));
        }

        private void FilterExchange(bool selected, bool all, Proto.Exchange exchange)
        {
            if (selected)
            {
                if (all)
                {
                    this.excludedExchanges.Clear();
                    for (int i = 1; i < this.Exchanges.Count; ++i)
                    {
                        this.Exchanges[i].SetIsSelected(true);
                    }
                }
                else
                {
                    this.excludedExchanges.Remove(exchange);
                }
            }
            else if (all)
            {
                for (int i = 1; i < this.Exchanges.Count; ++i)
                {
                    this.Exchanges[i].SetIsSelected(false);
                    this.excludedExchanges.Add(this.Exchanges[i].Item);
                }
            }
            else
            {
                this.Exchanges[0].SetIsSelected(false);
                this.excludedExchanges.Add(exchange);
            }
            ExchangesView.Refresh();
            MessageView.Refresh();
        }

        private void FilterType(bool selected, bool all, Proto.InfoType type)
        {
            if (selected)
            {
                if (all)
                {
                    this.excludedTypes.Clear();
                    for (int i = 1; i < this.Types.Count; ++i)
                    {
                        this.Types[i].SetIsSelected(true);
                    }
                }
                else
                {
                    this.excludedTypes.Remove(type);
                }
            }
            else if (all)
            {
                for (int i = 1; i < this.Types.Count; ++i)
                {
                    this.Types[i].SetIsSelected(false);
                    this.excludedTypes.Add(this.Types[i].Item);
                }
            }
            else
            {
                this.Types[0].SetIsSelected(false);
                this.excludedTypes.Add(type);
            }
            TypesView.Refresh();
            MessageView.Refresh();
        }
    }
}
