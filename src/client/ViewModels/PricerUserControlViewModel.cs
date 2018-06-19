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
using System.Windows.Input;
using System.Windows.Threading;

namespace client.ViewModels
{
    public class PricerUserControlViewModel : BindableBase
    {
        public DelegateCommand EditCommand { get; set; }
        //public DelegateCommand<PricerItem> AddCommand { get; set; }
        public DelegateCommand DeleteCommand { get; set; }
        public DelegateCommand RefreshCommand { get; set; }
        public DelegateCommand SetCommand { get; set; }
        //public DelegateCommand SetAllCommand { get; set; }
        public DelegateCommand SelectAllCommand { get; set; }

        private ObservableCollection<PricerItem> pricers;
        public ObservableCollection<PricerItem> Pricers
        {
            get { return pricers; }
            set { SetProperty(ref pricers, value); }
        }

        private PricerItem selectedPricer;
        public PricerItem SelectedPricer
        {
            get { return selectedPricer; }
            set { SetProperty(ref selectedPricer, value); }
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
                    //AddCommand.RaiseCanExecuteChanged();
                    DeleteCommand.RaiseCanExecuteChanged();
                    //SetCommand.RaiseCanExecuteChanged();
                    //SetAllCommand.RaiseCanExecuteChanged();
                    SelectAllCommand.RaiseCanExecuteChanged();
                }
            }
        }        

        public IEnumerable<Instrument> GetHedgeUnderlyings()
        {
            var pm = Container.Resolve<ProductManager>(Exchange.ToString());
            return pm.GetHedgeUnderlyings();
        }

        public void AddPricer(string name, Instrument underlying, Proto.PricingModel model, int interval, Proto.UnderlyingTheoType theoType,
            int maxTickChange, double elastic, double elasticLimit, bool selectAllOptions)
        {
            var pm = Container.Resolve<ProductManager>(Exchange.ToString());
            if (pm != null)
            {
                var options = pm.GetOptionsByHedgeUnderlying(underlying);
                if (options != null)
                {
                    var item = new PricerItem(this.SetModified, name, underlying, model, interval, theoType, maxTickChange, elastic, elasticLimit, options.OrderBy(x => x.Id), selectAllOptions);

                    Proto.PricerReq req = new Proto.PricerReq();
                    req.Type = Proto.RequestType.Set;
                    req.Exchange = Exchange;

                    var pricer = new Proto.Pricer()
                    {
                        Name = name,
                        Underlying = underlying.Id,
                        Model = model,
                        Interval = interval,
                        TheoType = theoType,
                        WarnTickChange = maxTickChange,
                        Elastic = elastic,
                        ElasticLimit = elasticLimit,
                    };
                    pricer.Options.AddRange(from it in item.Options where it.Selected select it.Option.Id);
                    req.Pricers.Add(pricer);
                    var service = this.Container.Resolve<ServerService>(Exchange.ToString());
                    req.User = service.User;
                    service.Request(req);

                    this.pricers.Add(item);
                    SelectedPricer = item;
                }
            }
        }

        public PricerUserControlViewModel(Proto.Exchange exchange, IUnityContainer container, Dispatcher dispatcher)
        {
            this.Exchange = exchange;
            this.Container = container;
            //this.dispatcher = dispatcher;

            EditCommand = new DelegateCommand(this.EditExecute, this.CanEdit);
            //AddCommand = new DelegateCommand<PricerItem>(this.AddExecute);
            DeleteCommand = new DelegateCommand(this.DeleteExecute, this.CanDelete);
            RefreshCommand = new DelegateCommand(this.Refresh);
            SetCommand = new DelegateCommand(this.SetExecute, this.CanSet);
            //SetAllCommand = new DelegateCommand(this.SetAllExecute, this.CanSetAll);
            SelectAllCommand = new DelegateCommand(this.SelectAllExecute, this.CanSelectAll);

            Refresh();
        }

        private void EditExecute()
        {
            IsEditable = true;
        }

        private bool CanEdit()
        {
            return !IsEditable;
        }

        private void AddExecute(PricerItem item)
        {

        }

        private void DeleteExecute()
        {
            Proto.PricerReq req = new Proto.PricerReq();
            req.Type = Proto.RequestType.Del;
            req.Exchange = Exchange;

            req.Pricers.Add(new Proto.Pricer()
                {
                    Name = SelectedPricer.Name,
                    Underlying = SelectedPricer.Underlying.Id,
                });
            var service = this.Container.Resolve<ServerService>(Exchange.ToString());
            req.User = service.User;
            service.Request(req);

            int idx = this.pricers.IndexOf(SelectedPricer);
            this.pricers.RemoveAt(idx);
            if (idx < this.pricers.Count())
            {
                SelectedPricer = this.pricers[idx];
            }
            else
            {
                SelectedPricer = this.pricers.Last();
            }
        }

        private bool CanDelete()
        {
            return IsEditable && SelectedPricer != null;
        }

        private void SelectAllExecute()
        {
            if (this.SelectedPricer != null)
            {
                foreach (var item in this.SelectedPricer.Options)
                {
                    item.Selected = true;
                }
            }
        }

        private bool CanSelectAll()
        {
            return IsEditable;
        }

        private void SetExecute()
        {
            Proto.PricerReq req = new Proto.PricerReq();
            req.Type = Proto.RequestType.Set;
            req.Exchange = Exchange;

            foreach (var p in Pricers)
            {
                if (p.Modified)
                {
                    Proto.Pricer pricer = new Proto.Pricer()
                        {
                            Name = p.Name,
                            Underlying = p.Underlying.Id,
                            Model = p.Model,
                            Interval = p.Interval,
                            TheoType = p.UnderlyingTheoType,
                            WarnTickChange = p.MaxTickChange,
                            Elastic = p.Elastic,
                            ElasticLimit = p.ElasticLimit,
                        };
                    foreach (var it in p.Options)
                    {
                        it.Modified = false;
                        if (it.Selected)
                        {
                            pricer.Options.Add(it.Option.Id);
                        }
                    }
                    //pricer.Options.AddRange(from it in p.Options where it.Selected select it.Option.Id);
                    req.Pricers.Add(pricer);
                    p.Modified = false;
                }
            }

            var service = this.Container.Resolve<ServerService>(Exchange.ToString());
            req.User = service.User;
            service.Request(req);
            UnsetModified();
        }

        private bool CanSet()
        {
            return modified;
        }

        //private void SetAllExecute()
        //{

        //}

        //private bool CanSetAll()
        //{
        //    return (this.SelectedPricer != null) && this.SelectedPricer.Modified;
        //}

        private void Refresh()
        {
            var psm = Container.Resolve<PricerManager>(Exchange.ToString());
            if (psm != null)
            {
                var pm = Container.Resolve<ProductManager>(Exchange.ToString());
                if (pm != null)
                {
                    var items = new ObservableCollection<PricerItem>();
                    var pricings = psm.GetPricingSpecs();
                    pricings.Sort((x, y) => x.Name.CompareTo(y.Name));
                    foreach (var p in pricings)
                    {
                        var underlying = pm.FindId(p.Underlying);
                        if (underlying != null)
                        {
                            var options = pm.GetOptionsByHedgeUnderlying(underlying);
                            if (options != null)
                            {
                                var item = new PricerItem(this.SetModified, underlying, options.OrderBy(x => x.Id), p);
                                items.Add(item);
                            }
                        }
                    }
                    this.Pricers = items;
                    if (items.Count > 0)
                    {
                        SelectedPricer = items[0];
                    }
                }

                IsEditable = false;
                UnsetModified();
            }
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
        //private Dispatcher dispatcher;
    }

    public class PricerItem : BindableBase
    {
        private string name;
        public string Name
        {
            get { return name; }
            set { SetProperty(ref name, value); }
        }

        private Instrument underlying;
        public Instrument Underlying
        {
            get { return underlying; }
            set
            {
                if (SetProperty(ref underlying, value))
                {
                    Modified = true;
                }
            }
        }

        private Proto.PricingModel model;
        public Proto.PricingModel Model
        {
            get { return model; }
            set
            {
                if (SetProperty(ref model, value))
                {
                    Modified = true;
                }
            }
        }

        private int interval;
        public int Interval
        {
            get { return interval; }
            set
            {
                if (SetProperty(ref interval, value))
                {
                    Modified = true;
                }
            }
        }

        private Proto.UnderlyingTheoType underlyingTheoType;
        public Proto.UnderlyingTheoType UnderlyingTheoType
        {
            get { return underlyingTheoType; }
            set
            {
                if (SetProperty(ref underlyingTheoType, value))
                {
                    Modified = true;
                }
            }
        }

        private int maxTickChange;
        public int MaxTickChange
        {
            get { return maxTickChange; }
            set
            {
                if (SetProperty(ref maxTickChange, value))
                {
                    Modified = true;
                }
            }
        }

        private double elastic;
        public double Elastic
        {
            get { return elastic; }
            set
            {
                if (SetProperty(ref elastic, value))
                {
                    Modified = true;
                }
            }
        }

        private double elasticLimit;
        public double ElasticLimit
        {
            get { return elasticLimit; }
            set
            {
                if (SetProperty(ref elasticLimit, value))
                {
                    Modified = true;
                }
            }
        }

        private ObservableCollection<SelectedOptionItem> options;
        public ObservableCollection<SelectedOptionItem> Options
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
                    this.action();
                }
            }
        }

        private Action action;

        private void SetModified()
        {
            this.Modified = true;
        }

        public PricerItem(Action action, string name, Instrument underlying, Proto.PricingModel model, int interval, Proto.UnderlyingTheoType theoType,
            int maxTickChange, double elastic, double elasticLimit, IEnumerable<Option> options, bool selectAllOptions)
        {
            this.action = action;
            this.name = name;
            this.underlying = underlying;
            this.model = model;
            this.interval = interval;
            this.maxTickChange = maxTickChange;
            this.elastic = elastic;
            this.elasticLimit = elasticLimit;
            this.options = new ObservableCollection<SelectedOptionItem>();
            foreach (Option option in options)
            {
                this.options.Add(new SelectedOptionItem(this.SetModified, selectAllOptions, option));
            }
        }

        public PricerItem(Action action, Instrument underlying, IEnumerable<Option> options, Proto.Pricer p)
        {
            this.action = action;

            name = p.Name;
            this.underlying = underlying;
            model = p.Model;
            interval = p.Interval;
            underlyingTheoType = p.TheoType;
            maxTickChange = p.WarnTickChange;
            elastic = p.Elastic;
            elasticLimit = p.ElasticLimit;

            this.options = new ObservableCollection<SelectedOptionItem>();
            foreach (Option option in options)
            {
                this.options.Add(new SelectedOptionItem(this.SetModified, p.Options.Contains(option.Id), option));
            }
        }
    }

    public class SelectedOptionItem : BindableBase
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

        private bool selected;
        public bool Selected
        {
            get { return selected; }
            set
            {
                if (SetProperty(ref selected, value))
                {
                    Modified = true;
                }
            }
        }

        public Option Option { get; set; }

        public SelectedOptionItem(Action action, bool selected, Option option)
        {
            this.action = action;
            this.selected = selected;
            this.Option = option;
        }

        private Action action;
    }
}
