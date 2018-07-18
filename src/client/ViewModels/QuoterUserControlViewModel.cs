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
using System.Windows.Threading;

namespace client.ViewModels
{
    public class QuoterUserControlViewModel : BindableBase
    {
        public DelegateCommand EditCommand { get; set; }
        public DelegateCommand DeleteCommand { get; set; }
        public DelegateCommand RefreshCommand { get; set; }
        public DelegateCommand SetCommand { get; set; }
        public DelegateCommand SelectAllCommand { get; set; }

        private ObservableCollection<QuoterItem> quoters;
        public ObservableCollection<QuoterItem> Quoters
        {
            get { return quoters; }
            set { SetProperty(ref quoters, value); }
        }

        private bool isEditable;
        public bool IsEditable
        {
            get { return isEditable; }
            set
            {
                if (SetProperty(ref isEditable, value))
                {
                    EditCommand.RaiseCanExecuteChanged();
                    DeleteCommand.RaiseCanExecuteChanged();
                    SelectAllCommand.RaiseCanExecuteChanged();
                }
            }
        }

        private QuoterItem selectedQuoter;
        public QuoterItem SelectedQuoter
        {
            get { return selectedQuoter; }
            set
            {
                if (SetProperty(ref selectedQuoter, value))
                {
                    DeleteCommand.RaiseCanExecuteChanged();
                }
            }
        }        

        public QuoterUserControlViewModel(Proto.Exchange exchange, IUnityContainer container, Dispatcher dispatcher)
        {
            this.Exchange = exchange;
            this.Container = container;

            EditCommand = new DelegateCommand(this.EditExecute, this.CanEdit);
            DeleteCommand = new DelegateCommand(this.DeleteExecute, this.CanDelete);
            RefreshCommand = new DelegateCommand(this.Refresh);
            SetCommand = new DelegateCommand(this.SetExecute, this.CanSet);
            SelectAllCommand = new DelegateCommand(this.SelectAllExecute, this.CanSelectAll);

            Refresh();
        }

        public IEnumerable<Proto.Pricer> GetPricers()
        {
            var pm = Container.Resolve<PricerManager>(Exchange.ToString());
            return pm.GetPricers();
        }

        public bool IsQuoterNameExisted(string name)
        {
            foreach (var q in this.Quoters)
            {
                if (q.Name == name)
                    return true;
            }
            return false;
        }

        public void AddQuoter(Proto.Pricer pricer, Proto.QuoterSpec quoter, bool selectAll)
        {
            var pm = Container.Resolve<ProductManager>(Exchange.ToString());
            if (pm != null)
            {
                var underlying = pm.FindId(pricer.Underlying);
                var options = pm.GetOptionsByHedgeUnderlying(underlying);
                if (options != null)
                {
                    Proto.QuoterReq req = new Proto.QuoterReq()
                        {
                            Type = Proto.RequestType.Set,
                            Exchange = Exchange,
                        };
                    if (selectAll)
                    {
                        quoter.Options.AddRange(from op in options select op.Id);
                    }
                    req.Quoters.Add(quoter);
                    var service = this.Container.Resolve<ServerService>(Exchange.ToString());
                    req.User = service.User;
                    service.Request(req);

                    var item = new QuoterItem(this.SetModified, options.OrderBy(x => x.Underlying.Id).ThenBy(x => x.Strike).ThenBy(x => x.OptionType).ToList(), pricer, quoter, selectAll);
                    this.Quoters.Add(item);
                    SelectedQuoter = item;
                }
            }
        }

        private void EditExecute()
        {
            IsEditable = true;
        }

        private bool CanEdit()
        {
            return !IsEditable;
        }

        private void DeleteExecute()
        {
            Proto.QuoterReq req = new Proto.QuoterReq();
            req.Type = Proto.RequestType.Del;
            req.Exchange = Exchange;

            req.Quoters.Add(new Proto.QuoterSpec()
            {
                Name = SelectedQuoter.Name,
                //Underlying = SelectedQuoter.Pricer.Underlying,
            });
            var service = this.Container.Resolve<ServerService>(Exchange.ToString());
            req.User = service.User;
            service.Request(req);

            int idx = this.Quoters.IndexOf(SelectedQuoter);
            this.Quoters.RemoveAt(idx);
            if (idx < this.Quoters.Count)
            {
                SelectedQuoter = this.Quoters[idx];
            }
            else if (this.Quoters.Count > 0)
            {
                SelectedQuoter = this.Quoters.Last();
            }
            else
            {
                SelectedQuoter = null;
            }
        }

        private bool CanDelete()
        {
            return IsEditable && SelectedQuoter != null;
        }

        private void Refresh()
        {
            var productManager = Container.Resolve<ProductManager>(Exchange.ToString());
            var pm = Container.Resolve<PricerManager>(Exchange.ToString());
            var qm = Container.Resolve<QuoterManager>(Exchange.ToString());
            if (qm != null && pm != null)
            {
                var items = new ObservableCollection<QuoterItem>();
                var quoters = qm.GetQuoters();
                quoters.Sort((x, y) => x.Name.CompareTo(y.Name));
                foreach (var q in quoters)
                {
                    var underlying = productManager.FindId(q.Underlying);
                    if (underlying != null)
                    {
                        var options = productManager.GetOptionsByHedgeUnderlying(underlying);
                        if (options != null)
                        {
                            var pricer = pm.GetPricer(q.Underlying);
                            items.Add(new QuoterItem(this.SetModified, options.OrderBy(x => x.Underlying.Id).ThenBy(x => x.Strike).ThenBy(x => x.OptionType).ToList(), pricer, q));
                        }
                    }
                }

                this.Quoters = items;
                if (items.Count > 0)
                {
                    SelectedQuoter = items[0];
                }

                IsEditable = false;
                UnsetModified();
            }
        }

        private void SetExecute()
        {
            Proto.QuoterReq req = new Proto.QuoterReq()
                {
                    Type = Proto.RequestType.Set,
                    Exchange = Exchange,
                };

            foreach (var q in Quoters)
            {
                if (q.Modified)
                {
                    var quoter = new Proto.QuoterSpec()
                        {
                            Name = q.Name,
                            Pricer = q.Pricer.Name,
                            Underlying = q.Pricer.Underlying,
                            DeltaLimit = q.DeltaLimit,
                            OrderLimit = q.OrderLimit,
                            TradeLimit = q.TradeLimit,
                            BidVolume = q.BidVolume,
                            AskVolume = q.AskVolume,
                            ResponseVolume = q.ResponseVolume,
                            Depth = q.MaxDepth,
                            RefillTimes = q.RefillTimes,
                            WideSpread = q.WideSpread,
                            Protection = q.IsProtection,
                        };
                    foreach (var item in q.Options)
                    {
                        if (item.CallSelected)
                        {
                            quoter.Options.Add(item.Call.Id);
                        }
                        if (item.PutSelected)
                        {
                            quoter.Options.Add(item.Put.Id);
                        }
                        item.Modified = false;
                    }
                    req.Quoters.Add(quoter);
                    q.Modified = false;
                }
            }

            var service = this.Container.Resolve<ServerService>(Exchange.ToString());
            req.User = service.User;
            service.Request(req);
            IsEditable = false;
            UnsetModified();
        }

        private bool CanSet()
        {
            return modified;
        }

        private void SelectAllExecute()
        {
            if (this.SelectedQuoter != null)
            {
                foreach (var item in this.SelectedQuoter.Options)
                {
                    item.CallSelected = true;
                    item.PutSelected = true;
                }
            }
        }

        private bool CanSelectAll()
        {
            return IsEditable;
        }

        private void SetModified()
        {
            modified = true;
            SetCommand.RaiseCanExecuteChanged();
        }

        private void UnsetModified()
        {
            modified = false;
            SetCommand.RaiseCanExecuteChanged();
        }

        private bool modified = false;
        public Proto.Exchange Exchange { get; set; }
        public IUnityContainer Container { get; set; }
    }

    public class QuoterItem : BindableBase
    {
        public QuoterItem(Action action, List<Option> options, Proto.Pricer pricer, Proto.QuoterSpec quoter)
        {
            this.action = action;
            this.name = quoter.Name;
            this.pricer = pricer;
            this.orderLimit = quoter.OrderLimit;
            this.tradeLimit = quoter.TradeLimit;
            this.deltaLimit = quoter.DeltaLimit;
            this.bidVolume = quoter.BidVolume;
            this.askVolume = quoter.AskVolume;
            this.responseVolume = quoter.ResponseVolume;
            this.maxDepth = quoter.Depth;
            this.refillTimes = quoter.RefillTimes;
            this.wideSpread = quoter.WideSpread;
            this.isProtection = quoter.Protection;

            this.options = new ObservableCollection<SelectedOptionPairItem>();
            for (int i = 0; i < options.Count; i = i + 2)
            {
                Option call = options[i];
                Option put = options[i + 1];
                this.options.Add(new SelectedOptionPairItem(this.SetModified, quoter.Options.Contains(call.Id), quoter.Options.Contains(put.Id), call, put));
            }
        }

        public QuoterItem(Action action, List<Option> options, Proto.Pricer pricer, Proto.QuoterSpec quoter, bool selectAll)
        {
            this.action = action;
            this.name = quoter.Name;
            this.pricer = pricer;
            this.orderLimit = quoter.OrderLimit;
            this.tradeLimit = quoter.TradeLimit;
            this.deltaLimit = quoter.DeltaLimit;
            this.bidVolume = quoter.BidVolume;
            this.askVolume = quoter.AskVolume;
            this.responseVolume = quoter.ResponseVolume;
            this.maxDepth = quoter.Depth;
            this.refillTimes = quoter.RefillTimes;
            this.wideSpread = quoter.WideSpread;
            this.isProtection = quoter.Protection;

            this.options = new ObservableCollection<SelectedOptionPairItem>();
            for (int i = 0; i < options.Count; i = i+2)
            {
                this.options.Add(new SelectedOptionPairItem(this.SetModified, selectAll, selectAll, options[i], options[i + 1]));
            }
        }

        private void SetModified()
        {
            this.Modified = true;
        }

        private string name;
        public string Name
        {
            get { return name; }
            set { SetProperty(ref name, value); }
        }

        private Proto.Pricer pricer;
        public Proto.Pricer Pricer
        {
            get { return pricer; }
            set
            {
                if (SetProperty(ref pricer, value))
                {
                    Modified = true;
                }
            }
        }

        private int orderLimit;
        public int OrderLimit
        {
            get { return orderLimit; }
            set
            {
                if (SetProperty(ref orderLimit, value))
                {
                    Modified = true;
                }
            }
        }

        private int tradeLimit;
        public int TradeLimit
        {
            get { return tradeLimit; }
            set
            {
                if (SetProperty(ref tradeLimit, value))
                {
                    Modified = true;
                }
            }
        }

        private double deltaLimit;
        public double DeltaLimit
        {
            get { return deltaLimit; }
            set
            {
                if (SetProperty(ref deltaLimit, value))
                {
                    Modified = true;
                }
            }
        }

        private int bidVolume;
        public int BidVolume
        {
            get { return bidVolume; }
            set
            {
                if (SetProperty(ref bidVolume, value))
                {
                    Modified = true;
                }
            }
        }

        private int askVolume;
        public int AskVolume
        {
            get { return askVolume; }
            set
            {
                if (SetProperty(ref askVolume, value))
                {
                    Modified = true;
                }
            }
        }

        private int responseVolume;
        public int ResponseVolume
        {
            get { return responseVolume; }
            set
            {
                if (SetProperty(ref responseVolume, value))
                {
                    Modified = true;
                }
            }
        }

        private int maxDepth;
        public int MaxDepth
        {
            get { return maxDepth; }
            set
            {
                if (SetProperty(ref maxDepth, value))
                {
                    Modified = true;
                }
            }
        }

        private int refillTimes;
        public int RefillTimes
        {
            get { return refillTimes; }
            set
            {
                if (SetProperty(ref refillTimes, value))
                {
                    Modified = true;
                }
            }
        }

        private bool wideSpread;
        public bool WideSpread
        {
            get { return wideSpread; }
            set
            {
                if (SetProperty(ref wideSpread, value))
                {
                    Modified = true;
                }
            }
        }        

        private bool isProtection;
        public bool IsProtection
        {
            get { return isProtection; }
            set
            {
                if (SetProperty(ref isProtection, value))
                {
                    Modified = true;
                }
            }
        }

        private ObservableCollection<SelectedOptionPairItem> options;
        public ObservableCollection<SelectedOptionPairItem> Options
        {
            get { return options; }
            set { SetProperty(ref options, value); }
        }

        private bool modified;
        public bool Modified
        {
            get { return modified; }
            set
            {
                if (SetProperty(ref modified, value))
                {
                    action();
                }
            }
        }

        private Action action;
    }

    public class SelectedOptionPairItem : BindableBase
    {
        private bool modified;
        public bool Modified
        {
            get { return modified; }
            set
            {
                if (SetProperty(ref modified, value))
                {
                    action();
                }
            }
        }

        private bool callSelected;
        public bool CallSelected
        {
            get { return callSelected; }
            set
            {
                if (SetProperty(ref callSelected, value))
                {
                    Modified = true;
                }
            }
        }

        private bool putSelected;
        public bool PutSelected
        {
            get { return putSelected; }
            set
            {
                if (SetProperty(ref putSelected, value))
                {
                    Modified = true;
                }
            }
        }


        public Option Call { get; set; }

        public Option Put { get; set; }

        public SelectedOptionPairItem(Action action, bool callSelected, bool putSelected, Option call, Option put)
        {
            this.action = action;
            this.callSelected = callSelected;
            this.putSelected = putSelected;
            this.Call = call;
            this.Put = put;
        }

        private Action action;
    }
}
