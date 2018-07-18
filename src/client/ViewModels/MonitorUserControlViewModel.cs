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
using System.Windows.Forms;
using System.Windows.Threading;

namespace client.ViewModels
{
    public class MonitorUserControlViewModel : BindableBase
    {
        public DelegateCommand PlayActiveCommand { get; set; }
        public DelegateCommand PlayAllCommand { get; set; }
        public DelegateCommand PlayQuotersCommand { get; set; }
        public DelegateCommand PlayHittersCommand { get; set; }
        public DelegateCommand PlayDimersCommand { get; set; }
        public DelegateCommand PlayDummyQuotersCommand { get; set; }
        public DelegateCommand StopAllCommand { get; set; }
        public DelegateCommand StopQuotersCommand { get; set; }
        public DelegateCommand StopHittersCommand { get; set; }
        public DelegateCommand StopDimersCommand { get; set; }
        public DelegateCommand StopDummyQuotersCommand { get; set; }

        private Proto.Exchange exchange;
        public Proto.Exchange Exchange
        {
            get { return exchange; }
            set { SetProperty(ref exchange, value); }
        }

        private ObservableCollection<MonitorItem> monitors;
        public ObservableCollection<MonitorItem> Monitors
        {
            get { return monitors; }
            set { SetProperty(ref monitors, value); }
        }

        public MonitorUserControlViewModel(Proto.Exchange exchange, IUnityContainer container, Dispatcher dispatcher)
        {
            this.exchange = exchange;
            this.container = container;
            this.dispatcher = dispatcher;

            this.monitors = new ObservableCollection<MonitorItem>();
            this.items = new Dictionary<string, MonitorItem>[]
                {
                    new Dictionary<string, MonitorItem>(),
                    new Dictionary<string, MonitorItem>(),
                    new Dictionary<string, MonitorItem>(),
                    new Dictionary<string, MonitorItem>(),
                };
            var qm = this.container.Resolve<QuoterManager>(exchange.ToString());
            var sm = this.container.Resolve<StrategySwitchManager>(exchange.ToString());
            if (qm != null && sm != null)
            {
                var quoters = qm.GetQuoters();
                foreach (var q in quoters.OrderBy(x => x.Name))
                {
                    var switches = new Dictionary<string, Proto.StrategySwitch>();
                    foreach (var op in q.Options)
                    {
                        switches[op] = sm.GetStrategySwitch(Proto.StrategyType.Quoter, op);
                    }
                    var item = new MonitorItem(this.exchange, this.container, this.dispatcher, switches)
                        {
                            Name = q.Name,
                            Underlying = q.Underlying,
                            Type = Proto.StrategyType.Quoter,
                            OptionNum = q.Options.Count,
                            DeltaLimit = q.DeltaLimit,
                            OrderLimit = q.OrderLimit,
                            TradeLimit = q.TradeLimit,
                        };
                    this.monitors.Add(item);
                    items[(int)Proto.StrategyType.Quoter][q.Name] = item;
                }

                PlayActiveCommand = new DelegateCommand(this.PlayActiveExecute);
                PlayAllCommand = new DelegateCommand(this.PlayAllExecute);
                PlayQuotersCommand = new DelegateCommand(this.PlayQuotersExecute);
                PlayHittersCommand = new DelegateCommand(this.PlayHittersExecute);
                PlayDimersCommand = new DelegateCommand(this.PlayDimersExecute);
                PlayDummyQuotersCommand = new DelegateCommand(this.PlayDummyQuotersExecute);
                StopAllCommand = new DelegateCommand(this.StopAllExecute);
                StopQuotersCommand = new DelegateCommand(this.StopQuotersExecute);
                StopHittersCommand = new DelegateCommand(this.StopHittersExecute);
                StopDimersCommand = new DelegateCommand(this.StopDimersExecute);
                StopDummyQuotersCommand = new DelegateCommand(this.StopDummyQuotersExecute);

                var epm = container.Resolve<ExchangeParameterManager>(exchange.ToString());
                if (epm != null)
                {
                    var interval = epm.GetStopTime();
                    if (interval != TimeSpan.Zero)
                    {
                        timer = new DispatcherTimer(interval, DispatcherPriority.Normal, timer_Elapsed, this.dispatcher);
                        timer.Start();
                    }
                }
            }
        }

        public void RefreshQuoter(Proto.QuoterSpec q)
        {
            for(int i = 0; i < this.Monitors.Count; ++i)
            {
                if (q.Name == this.Monitors[i].Name)
                {
                    var sm = this.container.Resolve<StrategySwitchManager>(exchange.ToString());
                    var switches = new Dictionary<string, Proto.StrategySwitch>();
                    foreach (var op in q.Options)
                    {
                        switches[op] = sm.GetStrategySwitch(Proto.StrategyType.Quoter, op);
                    }
                    this.Monitors[i].SetStrategySwitches(switches);
                    this.Monitors[i].Underlying = q.Underlying;
                    this.Monitors[i].OptionNum = q.Options.Count;
                    this.Monitors[i].DeltaLimit = q.DeltaLimit;
                    this.Monitors[i].OrderLimit = q.OrderLimit;
                    this.Monitors[i].TradeLimit = q.TradeLimit;
                    break;
                }
                else if (q.Name.CompareTo(this.Monitors[i].Name) < 0 || this.Monitors[i].Type != Proto.StrategyType.Quoter)
                {
                    var sm = this.container.Resolve<StrategySwitchManager>(exchange.ToString());
                    var switches = new Dictionary<string, Proto.StrategySwitch>();
                    foreach (var op in q.Options)
                    {
                        switches[op] = sm.GetStrategySwitch(Proto.StrategyType.Quoter, op);
                    }
                    var item = new MonitorItem(this.exchange, this.container, this.dispatcher, switches)
                    {
                        Name = q.Name,
                        Underlying = q.Underlying,
                        Type = Proto.StrategyType.Quoter,
                        OptionNum = q.Options.Count,
                        DeltaLimit = q.DeltaLimit,
                        OrderLimit = q.OrderLimit,
                        TradeLimit = q.TradeLimit,
                    };
                    this.Monitors.Insert(i, item);
                    items[(int)Proto.StrategyType.Quoter][q.Name] = item;
                    break;
                }
            }
        }

        public void RefreshStrategySwitch(Proto.StrategySwitch sw)
        {
            this.dispatcher.Invoke((MethodInvoker)delegate
            {
                foreach (var kvp in this.items[(int)sw.Strategy])
                {
                    kvp.Value.SetStrategySwitch(sw);
                }
            });
        }

        public void RefreshStrategyStatistic(Proto.StrategyStatistic statistic)
        {
            MonitorItem item = null;
            if (this.items[(int)statistic.Type].TryGetValue(statistic.Name, out item))
            {
                this.dispatcher.Invoke((MethodInvoker)delegate
                {
                    item.SetStrategyStatistic(statistic);
                });
            }
        }

        private void PlayActiveExecute()
        {
            Proto.StrategyOperateReq req = new Proto.StrategyOperateReq();
            req.Type = Proto.RequestType.Set;
            req.Exchange = exchange;
            
            foreach (var item in this.Monitors)
            {
                if (item.Status == Proto.StrategyStatus.Running)
                {
                    req.Operates.Add(new Proto.StrategyOperate()
                        {
                            Name = item.Name,
                            Underlying = item.Underlying,
                            Operate = Proto.StrategyOperation.Start,
                        });
                }
            }

            if (req.Operates.Count > 0)
            {
                var server = this.container.Resolve<ServerService>(this.exchange.ToString());
                req.User = server.User;
                server.Request(req);
            }
        }

        private void PlayAllExecute()
        {
            PlayStopAllExecute(Proto.StrategyOperation.Start);
        }

        private void StopAllExecute()
        {
            PlayStopAllExecute(Proto.StrategyOperation.Stop);
        }

        private void PlayStopAllExecute(Proto.StrategyOperation op)
        {
            Proto.StrategyOperateReq req = new Proto.StrategyOperateReq();
            req.Type = Proto.RequestType.Set;
            req.Exchange = exchange;

            foreach (var item in this.Monitors)
            {
                req.Operates.Add(new Proto.StrategyOperate()
                    {
                        Name = item.Name,
                        Underlying = item.Underlying,
                        Operate = op,
                    });
            }

            if (req.Operates.Count > 0)
            {
                var server = this.container.Resolve<ServerService>(this.exchange.ToString());
                req.User = server.User;
                server.Request(req);
            }
        }

        private void PlayQuotersExecute()
        {
            PlayStopExecute(Proto.StrategyType.Quoter, Proto.StrategyOperation.Start);
        }

        private void PlayHittersExecute()
        {
            PlayStopExecute(Proto.StrategyType.Hitter, Proto.StrategyOperation.Start);
        }

        private void PlayDimersExecute()
        {
            PlayStopExecute(Proto.StrategyType.Dimer, Proto.StrategyOperation.Start);
        }

        private void PlayDummyQuotersExecute()
        {
            PlayStopExecute(Proto.StrategyType.DummyQuoter, Proto.StrategyOperation.Start);
        }

        private void StopQuotersExecute()
        {
            PlayStopExecute(Proto.StrategyType.Quoter, Proto.StrategyOperation.Stop);
        }

        private void StopHittersExecute()
        {
            PlayStopExecute(Proto.StrategyType.Hitter, Proto.StrategyOperation.Stop);
        }

        private void StopDimersExecute()
        {
            PlayStopExecute(Proto.StrategyType.Dimer, Proto.StrategyOperation.Stop);
        }

        private void StopDummyQuotersExecute()
        {
            PlayStopExecute(Proto.StrategyType.DummyQuoter, Proto.StrategyOperation.Stop);
        }

        private void PlayStopExecute(Proto.StrategyType strategy, Proto.StrategyOperation op)
        {
            Proto.StrategyOperateReq req = new Proto.StrategyOperateReq();
            req.Type = Proto.RequestType.Set;
            req.Exchange = exchange;

            foreach (var kvp in this.items[(int)strategy])
            {
                req.Operates.Add(new Proto.StrategyOperate()
                {
                    Name = kvp.Value.Name,
                    Underlying = kvp.Value.Underlying,
                    Operate = op,
                });
            }

            if (req.Operates.Count > 0)
            {
                var server = this.container.Resolve<ServerService>(this.exchange.ToString());
                req.User = server.User;
                server.Request(req);
            }
        }

        private void timer_Elapsed(object sender, EventArgs e)
        {
            StopAllExecute();

            var epm = container.Resolve<ExchangeParameterManager>(exchange.ToString());
            if (epm != null)
            {
                var interval = epm.GetStopTime();
                if (interval != TimeSpan.Zero)
                {
                    this.timer.Interval = epm.GetStopTime();
                    return;
                }
            }
            timer.Stop();
        }

        private IUnityContainer container;
        private Dispatcher dispatcher;
        private Dictionary<string, MonitorItem>[] items;
        private DispatcherTimer timer;
    }

    public class MonitorItem : BindableBase
    {
        public MonitorItem(Proto.Exchange exchange, IUnityContainer container, Dispatcher dispatcher, Dictionary<string, Proto.StrategySwitch> switches)
        {
            this.exchange = exchange;
            this.container = container;
            SetStrategySwitches(switches);
            this.PlayCommand = new DelegateCommand(this.PlayExecute);
            this.PauseCommand = new DelegateCommand(this.PauseExecute, this.CanPauseStop);
            this.StopCommand = new DelegateCommand(this.StopExecute, this.CanPauseStop);

            timer = new DispatcherTimer(interval, DispatcherPriority.Normal, timer_Elapsed, dispatcher);
        }

        public void SetStrategySwitches(Dictionary<string, Proto.StrategySwitch> switches)
        {
            this.switches = switches;
            int bidOnNum = 0, askOnNum = 0;
            foreach (var kvp in switches)
            {
                if (kvp.Value != null)
                {
                    if (kvp.Value.IsBid) ++bidOnNum;
                    if (kvp.Value.IsAsk) ++askOnNum;
                }
            }
            this.BidOnNum = bidOnNum;
            this.AskOnNum = askOnNum;
        }

        public void SetStrategySwitch(Proto.StrategySwitch sw)
        {
            Proto.StrategySwitch s = null;
            if (this.switches.TryGetValue(sw.Option, out s))
            {
                if (s != null)
                {
                    if (s.IsBid)
                    {
                        if (!sw.IsBid) --BidOnNum;
                    }
                    else
                    {
                        if (sw.IsBid) ++BidOnNum;
                    }
                    if (s.IsAsk)
                    {
                        if (!sw.IsAsk) --AskOnNum;
                    }
                    else
                    {
                        if (sw.IsAsk) ++AskOnNum;
                    }
                }
                else
                {
                    if (sw.IsBid) ++BidOnNum;
                    if (sw.IsAsk) ++AskOnNum;
                }
                this.switches[sw.Option] = sw;
            }
        }

        public void SetStrategyStatistic(Proto.StrategyStatistic statistic)
        {
            Status = statistic.Status;
            if (statistic.Status == Proto.StrategyStatus.Running)
            {
                Delta = statistic.Delta;
                OrderNum = statistic.Orders;
                TradeNum = statistic.Trades;
                lastTime = DateTime.Now;
            }
            else if (statistic.Status == Proto.StrategyStatus.Started)
            {
                timer.Start();
                lastTime = DateTime.Now;
            }
            else if (statistic.Status == Proto.StrategyStatus.Stopped)
            {
                timer.Stop();
            }
        }

        private void PlayExecute()
        {
            OperateExecute(Proto.StrategyOperation.Start);
        }

        private void PauseExecute()
        {
            OperateExecute(Proto.StrategyOperation.Pause);
        }

        private void StopExecute()
        {
            OperateExecute(Proto.StrategyOperation.Stop);
        }

        private void OperateExecute(Proto.StrategyOperation op)
        {
            Proto.StrategyOperateReq req = new Proto.StrategyOperateReq();
            req.Type = Proto.RequestType.Set;
            req.Exchange = exchange;

            req.Operates.Add(new Proto.StrategyOperate()
            {
                Name = this.Name,
                Underlying = this.Underlying,
                Operate = op,
            });

            var server = this.container.Resolve<ServerService>(this.exchange.ToString());
            req.User = server.User;
            server.Request(req);
        }

        private bool CanPauseStop()
        {
            return this.Status == Proto.StrategyStatus.Running;
        }

        private void timer_Elapsed(object sender, EventArgs e)
        {
            if (this.Status == Proto.StrategyStatus.Running && (DateTime.Now - lastTime) > interval)
            {
                this.Status = Proto.StrategyStatus.Lost;
            }
        }

        public DelegateCommand PlayCommand { get; set; }
        public DelegateCommand PauseCommand { get; set; }
        public DelegateCommand StopCommand { get; set; }

        private string name;
        public string Name
        {
            get { return name; }
            set { SetProperty(ref name, value); }
        }

        private string underlying;
        public string Underlying
        {
            get { return underlying; }
            set { SetProperty(ref underlying, value); }
        }

        private Proto.StrategyType type;
        public Proto.StrategyType Type
        {
            get { return type; }
            set { SetProperty(ref type, value); }
        }

        private Proto.StrategyStatus status = Proto.StrategyStatus.Stopped;
        public Proto.StrategyStatus Status
        {
            get { return status; }
            set
            {
                if (SetProperty(ref status, value))
                {
                    PauseCommand.RaiseCanExecuteChanged();
                    StopCommand.RaiseCanExecuteChanged();
                }
            }
        }

        private int refilledNum;
        public int RefilledNum
        {
            get { return refilledNum; }
            set { SetProperty(ref refilledNum, value); }
        }

        private int bidOnNum;
        public int BidOnNum
        {
            get { return bidOnNum; }
            set { SetProperty(ref bidOnNum, value); }
        }

        private int askOnNum;
        public int AskOnNum
        {   
            get { return askOnNum; }
            set { SetProperty(ref askOnNum, value); }
        }

        private int optionNum;
        public int OptionNum
        {
            get { return optionNum; }
            set { SetProperty(ref optionNum, value); }
        }

        private double delta;
        public double Delta
        {
            get { return delta; }
            set { SetProperty(ref delta, value); }
        }

        private double deltaLimit;
        public double DeltaLimit
        {
            get { return deltaLimit; }
            set { SetProperty(ref deltaLimit, value); }
        }

        private int orderNum;
        public int OrderNum
        {
            get { return orderNum; }
            set { SetProperty(ref orderNum, value); }
        }

        private int orderLimit;
        public int OrderLimit
        {
            get { return orderLimit; }
            set { SetProperty(ref orderLimit, value); }
        }

        private int tradeNum;
        public int TradeNum
        {
            get { return tradeNum; }
            set { SetProperty(ref tradeNum, value); }
        }

        private int tradeLimit;
        public int TradeLimit
        {
            get { return tradeLimit; }
            set { SetProperty(ref tradeLimit, value); }
        }

        private Proto.Exchange exchange;
        private IUnityContainer container;
        private Dictionary<string, Proto.StrategySwitch> switches;

        TimeSpan interval = TimeSpan.FromSeconds(10);
        DateTime lastTime;
        DispatcherTimer timer;
    }
}
