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
using System.Windows.Forms;
using System.Windows.Threading;

namespace client.ViewModels
{
    class OrderWindowViewModel : BindableBase
    {
        public OrderWindowViewModel(IUnityContainer container, Dispatcher dispatcher)
        {
            this.container = container;
            this.dispatcher = dispatcher;
            this.CancelCommand = new DelegateCommand(this.CancelExecute, this.CanCancel);
            this.CancelAllCommand = new DelegateCommand(this.CancelAllExecute);

            this.container.Resolve<EventAggregator>().GetEvent<PubSubEvent<List<Proto.Exchange>>>().Subscribe(this.StartWindow, ThreadOption.PublisherThread);

        }

        public DelegateCommand CancelCommand { get; set; }
        public DelegateCommand CancelAllCommand { get; set; }

        private RangeObservableCollection<OrderItem> items;
        public RangeObservableCollection<OrderItem> Items
        {
            get { return items; }
            set { SetProperty(ref items, value); }
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
                    items.Add(item);
                }
            }

            this.Items = items;
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

        private IUnityContainer container;
        private Dispatcher dispatcher;
        private int index = 0;
        private Dictionary<ulong, OrderItem> orders;
        private Dictionary<Proto.Exchange, ProductManager> productManagers;
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
