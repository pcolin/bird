using client.Models;
using Microsoft.Practices.Unity;
using Prism.Commands;
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
    class OrderWindowViewModel : BindableBase
    {
        public OrderWindowViewModel(IUnityContainer container, Dispatcher dispatcher)
        {
            var exchanges = new ObservableCollection<FilterItem<Proto.Exchange>>();
            exchanges.Add(new FilterItem<Proto.Exchange>(this.FilterExchange));
            this.Exchanges = exchanges;

            //Func<Instrument, string> instrumentFunc = i => i.Id;
            var instruments = new ObservableCollection<FilterItem<Instrument>>();
            instruments.Add(new FilterItem<Instrument>(this.FilterInstrument));
            this.Instruments = instruments;

            var underlyings = new ObservableCollection<FilterItem<Instrument>>();
            underlyings.Add(new FilterItem<Instrument>(this.FilterUnderlying));
            this.Underlyings = underlyings;

            //Func<Proto.Side, string> sideFunc = s => s.ToString();
            var sides = new ObservableCollection<FilterItem<Proto.Side>>();
            sides.Add(new FilterItem<Proto.Side>(this.FilterSide));
            foreach (var s in Enum.GetValues(typeof(Proto.Side)))
            {
                sides.Add(new FilterItem<Proto.Side>(this.FilterSide, x => x.ToString(), (Proto.Side)s));
            }
            this.Sides = sides;

            //Func<Proto.OrderStatus, string> statusFunc = s => s.ToString();
            var statuses = new ObservableCollection<FilterItem<Proto.OrderStatus>>();
            statuses.Add(new FilterItem<Proto.OrderStatus>(this.FilterStatus));
            foreach (var s in Enum.GetValues(typeof(Proto.OrderStatus)))
            {
                statuses.Add(new FilterItem<Proto.OrderStatus>(this.FilterStatus, x => x.ToString(), (Proto.OrderStatus)s));
            }
            this.Statuses = statuses;

            //Func<Proto.StrategyType, string> strategyFunc = s => s.ToString();
            var strategies = new ObservableCollection<FilterItem<Proto.StrategyType>>();
            strategies.Add(new FilterItem<Proto.StrategyType>(this.FilterStrategy));
            foreach (var s in Enum.GetValues(typeof(Proto.StrategyType)))
            {
                strategies.Add(new FilterItem<Proto.StrategyType>(this.FilterStrategy, x => x.ToString(), (Proto.StrategyType)s));
            }
            this.Strategies = strategies;

            //Func<Proto.InstrumentType, string> typeFunc = t => t.ToString();
            var types = new ObservableCollection<FilterItem<Proto.InstrumentType>>();
            types.Add(new FilterItem<Proto.InstrumentType>(this.FilterType));
            foreach (var t in Enum.GetValues(typeof(Proto.InstrumentType)))
            {
                types.Add(new FilterItem<Proto.InstrumentType>(this.FilterType, x => x.ToString(), (Proto.InstrumentType)t));
            }
            this.Types = types;

            this.Items = new RangeObservableCollection<OrderItem>();

            this.container = container;
            this.dispatcher = dispatcher;
            this.CancelCommand = new DelegateCommand(this.CancelExecute, this.CanCancel);
            this.CancelAllCommand = new DelegateCommand(this.CancelAllExecute);

            this.container.Resolve<EventAggregator>().GetEvent<PubSubEvent<List<Proto.Exchange>>>().Subscribe(this.StartWindow, ThreadOption.PublisherThread);

        }

        public DelegateCommand CancelCommand { get; set; }
        public DelegateCommand CancelAllCommand { get; set; }

        public ICollectionView OrderView { get; set; }

        private RangeObservableCollection<OrderItem> items;
        public RangeObservableCollection<OrderItem> Items
        {
            get { return items; }
            set
            {
                if (SetProperty(ref items, value))
                {
                    OrderView = CollectionViewSource.GetDefaultView(value);
                    OrderView.Filter = this.Filter;
                }
            }
        }

        private OrderItem selectedItem;
        public OrderItem SelectedItem
        {
            get { return selectedItem; }
            set
            {
                if (SetProperty(ref selectedItem, value))
                {
                    this.CancelCommand.RaiseCanExecuteChanged();
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

        public ICollectionView InstrumentsView { get; set; }
        private ObservableCollection<FilterItem<Instrument>> instruments;
        private ObservableCollection<FilterItem<Instrument>> Instruments
        {
            get { return instruments; }
            set
            {
                if (SetProperty(ref instruments, value))
                {
                    InstrumentsView = CollectionViewSource.GetDefaultView(value);
                }
            }
        }

        public ICollectionView SidesView { get; set; }
        private ObservableCollection<FilterItem<Proto.Side>> sides;
        private ObservableCollection<FilterItem<Proto.Side>> Sides
        {
            get { return sides; }
            set
            {
                if (SetProperty(ref sides, value))
                {
                    SidesView = CollectionViewSource.GetDefaultView(value);
                }
            }
        }

        public ICollectionView StatusesView { get; set; }
        private ObservableCollection<FilterItem<Proto.OrderStatus>> statuses;
        private ObservableCollection<FilterItem<Proto.OrderStatus>> Statuses
        {
            get { return statuses; }
            set
            {
                if (SetProperty(ref statuses, value))
                {
                    StatusesView = CollectionViewSource.GetDefaultView(value);
                }
            }
        }

        public ICollectionView StrategiesView { get; set; }
        private ObservableCollection<FilterItem<Proto.StrategyType>> strategies;
        public ObservableCollection<FilterItem<Proto.StrategyType>> Strategies
        {
            get { return strategies; }
            set
            {
                if (SetProperty(ref strategies, value))
                {
                    StrategiesView = CollectionViewSource.GetDefaultView(value);
                }
            }
        }

        public ICollectionView TypesView { get; set; }
        private ObservableCollection<FilterItem<Proto.InstrumentType>> types;
        private ObservableCollection<FilterItem<Proto.InstrumentType>> Types
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

        public ICollectionView UnderlyingsView { get; set; }
        private ObservableCollection<FilterItem<Instrument>> underlyings;
        private ObservableCollection<FilterItem<Instrument>> Underlyings
        {
            get { return underlyings; }
            set
            {
                if (SetProperty(ref underlyings, value))
                {
                    UnderlyingsView = CollectionViewSource.GetDefaultView(value);
                }
            }
        }        
        
        private void StartWindow(List<Proto.Exchange> exchanges)
        {
            this.productManagers = new Dictionary<Proto.Exchange, ProductManager>();
            List<Proto.Order> orders = new List<Proto.Order>();
            foreach (var exchange in exchanges)
            {
                var om = this.container.Resolve<OrderManager>(exchange.ToString());
                var pm = this.container.Resolve<ProductManager>(exchange.ToString());
                if (om != null && pm != null)
                {
                    orders.AddRange(om.GetOrders());
                    this.productManagers[exchange] = pm;

                    this.Exchanges.Add(new FilterItem<Proto.Exchange>(this.FilterExchange, x => x.ToString(), exchange));
                    foreach (var underlying in pm.GetUnderlyings().OrderBy(x => x.Id))
                    {
                        this.Underlyings.Add(new FilterItem<Instrument>(this.FilterUnderlying, x => x.Id, underlying));
                        this.Instruments.Add(new FilterItem<Instrument>(this.FilterInstrument, x => x.Id, underlying));
                        foreach (var option in pm.GetOptions(underlying).OrderBy(x => x.Id))
                        {
                            this.Instruments.Add(new FilterItem<Instrument>(this.FilterInstrument, x => x.Id, option));
                        }
                    }
                }
            }

            var items = new RangeObservableCollection<OrderItem>();
            this.orders = new Dictionary<ulong, OrderItem>();
            //var sortedOrders = from order in orders orderby order.Time ascending select order;
            foreach (var order in orders.OrderBy(x => x.Time))
            {
                var inst = productManagers[order.Exchange].FindId(order.Instrument);
                if (inst != null)
                {
                    var item = new OrderItem(inst, order);
                    this.orders.Add(order.Id, item);
                    this.Items.Add(item);
                }
            }

            //this.Items = items;
            this.container.Resolve<EventAggregator>().GetEvent<PubSubEvent<Proto.OrderReq>>().Subscribe(this.ReceiveOrders, ThreadOption.BackgroundThread);
        }

        private void ReceiveOrders(Proto.OrderReq req)
        {            
            foreach (var order in req.Orders)
            {
                this.dispatcher.Invoke((MethodInvoker)delegate { ReceiveOrder(order); });
            }
        }

        private void ReceiveOrder(Proto.Order order)
        {
            const int removeThreshold = 5000;
            if (this.Items.Count > removeThreshold)
            {
                int count = 1000;
                while (count > 0)
                {
                    while (index < this.Items.Count && this.Items[index].Status != Proto.OrderStatus.Canceled) ++index;
                    if (index < this.Items.Count - 1)
                    {
                        int end = index + 1;
                        while (end < this.Items.Count && this.items[end].Status == Proto.OrderStatus.Canceled) ++end;
                        this.Items.RemoveRange(index, end - index);
                        count -= end - index;
                        ++index;
                    }
                    else
                    {
                        break;
                    }
                }
            }
            OrderItem item = null;
            if (this.orders.TryGetValue(order.Id, out item))
            {
                //if (order.Status == Proto.OrderStatus.Canceled)
                //{
                //    this.Items.Remove(item);
                //}
                //else
                {
                    item.ExchangeID = order.ExchangeId;
                    item.Status = order.Status;
                    item.Note = order.Note;
                    item.ExecutedVolume = order.ExecutedVolume;
                    item.Latency = item.Latency;
                    item.Order = order;
                }
            }
            else// if (order.Status != Proto.OrderStatus.Canceled)
            {
                ProductManager pm = null;
                if (this.productManagers.TryGetValue(order.Exchange, out pm))
                {
                    var inst = pm.FindId(order.Instrument);
                    if (inst != null)
                    {
                        item = new OrderItem(inst, order);
                        this.orders.Add(order.Id, item);
                        this.Items.Add(item);
                    }
                }
            }
        }

        private void CancelExecute()
        {
            Proto.OrderRequest req = new Proto.OrderRequest() { Action = Proto.OrderAction.Cancel, };
            req.Orders.Add(this.SelectedItem.Order);

            var server = this.container.Resolve<ServerService>(this.SelectedItem.Instrument.Exchange.ToString());
            server.Request(req);
        }

        private bool CanCancel()
        {
            return this.SelectedItem != null && (OrderManager.IsInactive(this.SelectedItem.Order) == false);
        }

        private void CancelAllExecute()
        {
            Proto.OrderRequest req = new Proto.OrderRequest() { Action = Proto.OrderAction.Cancel, };
            foreach (var item in this.Items)
            {
                if (OrderManager.IsInactive(item.Order) == false)
                {
                    req.Orders.Add(item.Order);
                }
            }
            if (req.Orders.Count > 0)
            {
                var server = this.container.Resolve<ServerService>(this.SelectedItem.Instrument.Exchange.ToString());
                server.Request(req);
            }
        }

        private bool Filter(object item)
        {
            var it = item as OrderItem;
            return !(excludedExchanges.Contains(it.Instrument.Exchange) || excludedInstruments.Contains(it.Instrument) || excludedSides.Contains(it.Side) || excludedStatuses.Contains(it.Status) ||
                excludedStrategies.Contains(it.Strategy) || excludedTypes.Contains(it.Instrument.Type) || excludedUnderlyings.Contains(it.Instrument.Underlying));
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
            OrderView.Refresh();
        }

        private void FilterInstrument(bool selected, bool all, Instrument instrument)
        {
            if (selected)
            {
                if (all)
                {
                    this.excludedInstruments.Clear();
                    for (int i = 1; i < this.Instruments.Count; ++i)
                    {
                        this.Instruments[i].SetIsSelected(true);
                    }
                }
                else
                {
                    this.excludedInstruments.Remove(instrument);
                }
            }
            else if (all)
            {
                for (int i = 1; i < this.Instruments.Count; ++i)
                {
                    this.Instruments[i].SetIsSelected(false);
                    this.excludedInstruments.Add(this.Instruments[i].Item);
                }
            }
            else
            {
                this.Instruments[0].SetIsSelected(false);
                this.excludedInstruments.Add(instrument);
            }
            InstrumentsView.Refresh();
            OrderView.Refresh();
        }

        private void FilterUnderlying(bool selected, bool all, Instrument underlying)
        {
            if (selected)
            {
                if (all)
                {
                    this.excludedUnderlyings.Clear();
                    for (int i = 1; i < this.Underlyings.Count; ++i)
                    {
                        this.Underlyings[i].SetIsSelected(true);
                    }
                }
                else
                {
                    this.excludedUnderlyings.Remove(underlying);
                }
            }
            else if (all)
            {
                for (int i = 1; i < this.Underlyings.Count; ++i)
                {
                    this.Underlyings[i].SetIsSelected(false);
                    this.excludedUnderlyings.Add(this.Underlyings[i].Item);
                }
            }
            else
            {
                this.Underlyings[0].SetIsSelected(false);
                this.excludedUnderlyings.Add(underlying);
            }
            UnderlyingsView.Refresh();
            OrderView.Refresh();
        }

        private void FilterSide(bool selected, bool all, Proto.Side side)
        {
            if (selected)
            {
                if (all)
                {
                    this.excludedSides.Clear();
                    for (int i = 1; i < this.Sides.Count; ++i)
                    {
                        this.Sides[i].SetIsSelected(true);
                    }
                }
                else
                {
                    this.excludedSides.Remove(side);
                }
            }
            else if (all)
            {
                for (int i = 1; i < this.Sides.Count; ++i)
                {
                    this.Sides[i].SetIsSelected(false);
                    this.excludedSides.Add(side);
                }
            }
            else
            {
                this.Sides[0].SetIsSelected(true);
                this.excludedSides.Add(side);
            }
            SidesView.Refresh();
            OrderView.Refresh();
        }

        private void FilterStatus(bool selected, bool all, Proto.OrderStatus status)
        {
            if (selected)
            {
                if (all)
                {
                    this.excludedStatuses.Clear();
                    for (int i = 1; i < this.Statuses.Count; ++i)
                    {
                        this.Statuses[i].SetIsSelected(true);
                    }
                }
                else
                {
                    this.excludedStatuses.Remove(status);
                }
            }
            else if (all)
            {
                for (int i = 1; i < this.Statuses.Count; ++i)
                {
                    this.Statuses[i].SetIsSelected(false);
                    this.excludedStatuses.Add(status);
                }
            }
            else
            {
                this.Statuses[0].SetIsSelected(true);
                this.excludedStatuses.Add(status);
            }
            StatusesView.Refresh();
            OrderView.Refresh();
        }

        private void FilterStrategy(bool selected, bool all, Proto.StrategyType strategy)
        {
            if (selected)
            {
                if (all)
                {
                    this.excludedStrategies.Clear();
                    for (int i = 1; i < this.Strategies.Count; ++i)
                    {
                        this.Strategies[i].SetIsSelected(true);
                    }
                }
                else
                {
                    this.excludedStrategies.Remove(strategy);
                }
            }
            else if (all)
            {
                for (int i = 1; i < this.Strategies.Count; ++i)
                {
                    this.Strategies[i].SetIsSelected(false);
                    this.excludedStrategies.Add(strategy);
                }
            }
            else
            {
                this.Strategies[0].SetIsSelected(true);
                this.excludedStrategies.Add(strategy);
            }
            StrategiesView.Refresh();
            OrderView.Refresh();
        }

        private void FilterType(bool selected, bool all, Proto.InstrumentType type)
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
                    this.excludedTypes.Add(type);
                }
            }
            else
            {
                this.Types[0].SetIsSelected(true);
                this.excludedTypes.Add(type);
            }
            TypesView.Refresh();
            OrderView.Refresh();
        }

        private IUnityContainer container;
        private Dispatcher dispatcher;
        private int index = 0;
        private Dictionary<ulong, OrderItem> orders;
        private Dictionary<Proto.Exchange, ProductManager> productManagers;
        private HashSet<Proto.Exchange> excludedExchanges = new HashSet<Proto.Exchange>();
        private HashSet<Instrument> excludedInstruments = new HashSet<Instrument>();
        private HashSet<Proto.Side> excludedSides = new HashSet<Proto.Side>();
        private HashSet<Proto.OrderStatus> excludedStatuses = new HashSet<Proto.OrderStatus>();
        private HashSet<Proto.StrategyType> excludedStrategies = new HashSet<Proto.StrategyType>();
        private HashSet<Proto.InstrumentType> excludedTypes = new HashSet<Proto.InstrumentType>();
        private HashSet<Instrument> excludedUnderlyings = new HashSet<Instrument>();
    }

    class OrderItem : BindableBase
    {
        public OrderItem(Instrument instrument, Proto.Order order)
        {
            this.Instrument = instrument;
            if (instrument.Type == Proto.InstrumentType.Option)
            {
                this.Type = (instrument as Option).OptionType.ToString();
            }
            else
            {
                this.Type = instrument.Type.ToString();
            }
            this.ID = order.Id;
            this.Strategy = order.StrategyType;
            this.Time = new DateTime(1970, 1, 1, 0, 0, 0).ToLocalTime().AddMilliseconds(order.Time / 1000);
            this.Price = order.Price;
            this.Volume = order.Volume;
            this.Side = order.Side;
            this.ExchangeID = order.ExchangeId;
            this.Status = order.Status;
            this.Note = order.Note;
            this.ExecutedVolume = order.ExecutedVolume;
            this.Latency = order.Latency;

            this.Order = order;
        }

        public Proto.Order Order { get; set; }
        public ulong ID { get; private set; }
        public DateTime Time { get; private set; }
        public Instrument Instrument { get; set; }
        public Proto.StrategyType Strategy { get; set; }
        public string Type { get; private set; }
        public double Price { get; private set; }
        public int Volume { get; private set; }
        public Proto.Side Side { get; private set; }

        private string exchangeID;
        public string ExchangeID
        {
            get { return exchangeID; }
            set { SetProperty(ref exchangeID, value); }
        }        

        private Proto.OrderStatus status;
        public Proto.OrderStatus Status
        {
            get { return status; }
            set { SetProperty(ref status, value); }
        }

        private string note;
        public string Note
        {
            get { return note; }
            set { SetProperty(ref note, value); }
        }

        private int executedVolume;
        public int ExecutedVolume
        {
            get { return executedVolume; }
            set { SetProperty(ref executedVolume, value); }
        }

        private int latency;
        public int Latency
        {
            get { return latency; }
            set { SetProperty(ref latency, value); }
        }        
    }
}
