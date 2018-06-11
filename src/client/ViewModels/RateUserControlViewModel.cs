using client.Models;
using Microsoft.Practices.Unity;
using Prism.Commands;
using Prism.Mvvm;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Globalization;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Input;

namespace client.ViewModels
{
    class RateUserControlViewModel : BindableBase
    {
        public ICommand ModifyCommand { get; set; }
        public ICommand RefreshCommand { get; set; }
        public ICommand AddRateCommand { get; set; }
        public ICommand DeleteRateCommand { get; set; }
        
        private Proto.Exchange exchange;
        public Proto.Exchange Exchange
        {
            get { return exchange; }
            set { SetProperty(ref exchange, value); }
        }

        private int selectedRate;
        public int SelectedRate
        {
            get { return selectedRate; }
            set { SetProperty(ref selectedRate, value); }
        }

        private ObservableCollection<RateItem> rates;
        public ObservableCollection<RateItem> Rates
        {
            get { return rates; }
            set { SetProperty(ref rates, value); }
        }

        private bool modified = false;
        private IUnityContainer container;

        public RateUserControlViewModel(IUnityContainer container, Proto.Exchange exchange)
        {
            this.container = container;
            Exchange = exchange;
            ModifyCommand = new DelegateCommand(this.ModifyExecute, this.CanModify);
            RefreshCommand = new DelegateCommand(this.Refresh);
            AddRateCommand = new DelegateCommand(this.AddRateExecute);
            DeleteRateCommand = new DelegateCommand(this.DeleteRateExecute);
        }

        public void Refresh()
        {
            InterestRateManager manager = this.container.Resolve<InterestRateManager>(exchange.ToString());
            if (manager != null)
            {
                var rates = new List<RateItem>();
                manager.Foreach((d, r) => rates.Add(new RateItem(this.SetModified, d, r)));
                //rates.Sort((x, y) => x.Days.CompareTo(y.Days));
                Rates = new ObservableCollection<RateItem>(rates);
                SelectedRate = Rates.Count - 1;
                
                UnsetModified();
            }
        }

        private void ModifyExecute()
        {
            Proto.InterestRateReq req = new Proto.InterestRateReq();
            req.Type = Proto.RequestType.Set;
            
            foreach (RateItem rate in Rates)
            {
                Proto.InterestRate r = new Proto.InterestRate();
                r.Days = rate.Days;
                r.Rate = rate.Rate;
                req.Rates.Add(r);
            }

            var service = this.container.Resolve<ServerService>(exchange.ToString());
            req.User = service.User;
            service.Request(req);
            UnsetModified();
        }

        private bool CanModify()
        {
            return modified;
        }

        private void SetModified()
        {
            this.modified = true;
            (ModifyCommand as DelegateCommand).RaiseCanExecuteChanged();
        }

        private void UnsetModified()
        {
            this.modified = false;
            (ModifyCommand as DelegateCommand).RaiseCanExecuteChanged();
        }

        private void AddRateExecute()
        {
            Rates.Add(new RateItem(this.SetModified, 30, 0.05));
            SelectedRate = Rates.Count - 1;
            SetModified();
        }

        private void DeleteRateExecute()
        {
            int count = Rates.Count;
            if (SelectedRate < count)
            {
                Rates.RemoveAt(SelectedRate);
                if (SelectedRate == count - 1)
                {
                    --SelectedRate;
                }
            }
            SetModified();
        }
    }

    class RateItem : BindableBase
    {
        private int days;
        public int Days
        {
            get { return days; }
            set
            {
                if (SetProperty(ref days, value))
                {
                    this.action();
                }
            }
        }

        private double rate;
        public double Rate
        {
            get { return rate; }
            set
            {
                if (SetProperty(ref rate, value))
                {
                    this.action();
                }
            }
        }  
      
        public RateItem(Action action, int days, double rate)
        {
            this.days = days;
            this.rate = rate;
            this.action = action;
        }

        private Action action;
    }
}
