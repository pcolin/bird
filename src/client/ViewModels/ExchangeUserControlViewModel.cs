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
    class ExchangeUserControlViewModel : BindableBase
    {
        public ICommand ModifyCommand { get; set; }
        public ICommand RefreshCommand { get; set; }
        public ICommand AddHolidayCommand { get; set; }
        public ICommand DeleteHolidayCommand { get; set; }
        public ICommand AddTradingSessionCommand { get; set; }
        public ICommand DeleteTradingSessionCommand { get; set; }
        public ICommand AddMaturityTradingSessionCommand { get; set; }
        public ICommand DeleteMaturityTradingSessionCommand { get; set; }
        
        private Proto.Exchange exchange;
        public Proto.Exchange Exchange
        {
            get { return exchange; }
            set { SetProperty(ref exchange, value); }
        }

        private DateTime charmCutoff;
        public DateTime CharmCutoff
        {
            get { return charmCutoff; }
            set
            {
                if (SetProperty(ref charmCutoff, value))
                {
                    SetModified();
                }
            }        
        }

        private double qrDelay;
        public double QRDelay
        {
            get { return qrDelay; }
            set { SetProperty(ref qrDelay, value); }
        }

        private double qrTimeout;
        public double QRTimeout
        {
            get { return qrTimeout; }
            set { SetProperty(ref qrTimeout, value); }
        }

        private int qrVolume;
        public int QRVolume
        {
            get { return qrVolume; }
            set { SetProperty(ref qrVolume, value); }
        }

        private int selectedTradingSession;
        public int SelectedTradingSession
        {
            get { return selectedTradingSession; }
            set { SetProperty(ref selectedTradingSession, value); }
        }        

        private ObservableCollection<TradingSession> tradingSessions;
        public ObservableCollection<TradingSession> TradingSessions    
        {
            get { return tradingSessions; }
            set { SetProperty(ref tradingSessions, value); }
        }

        private int selectedMaturityTradingSession;
        public int SelectedMaturityTradingSession
        {
            get { return selectedMaturityTradingSession; }
            set { SetProperty(ref selectedMaturityTradingSession, value); }
        }
        
        private ObservableCollection<TradingSession> maturityTradingSessions;
        public ObservableCollection<TradingSession> MaturityTradingSessions
        {
            get { return maturityTradingSessions; }
            set { SetProperty(ref maturityTradingSessions, value); }
        }

        private int selectedHoliday;
        public int SelectedHoliday
        {
            get { return selectedHoliday; }
            set { SetProperty(ref selectedHoliday, value); }
        }        

        private ObservableCollection<HolidayItem> holidays;
        public ObservableCollection<HolidayItem> Holidays
        {
            get { return holidays; }
            set { SetProperty(ref holidays, value); }
        }

        private bool modified = false;
        private IUnityContainer container;

        public ExchangeUserControlViewModel(IUnityContainer container, Proto.Exchange exchange)
        {
            this.container = container;
            Exchange = exchange;
            ModifyCommand = new DelegateCommand(this.ModifyExecute, this.CanModify);
            RefreshCommand = new DelegateCommand(this.Refresh);

            AddHolidayCommand = new DelegateCommand(this.AddHolidayExecute);
            DeleteHolidayCommand = new DelegateCommand(this.DeleteHolidayExecute);
            AddTradingSessionCommand = new DelegateCommand(this.AddTradingSessionExecute);
            DeleteTradingSessionCommand = new DelegateCommand(this.DeleteTradingSessionExecute);
            AddMaturityTradingSessionCommand = new DelegateCommand(this.AddMaturityTradingSessionExecute);
            DeleteMaturityTradingSessionCommand = new DelegateCommand(this.DeleteMaturityTradingSessionExecute);
        }

        public void Refresh()
        {
            ExchangeParameterManager manager = this.container.Resolve<ExchangeParameterManager>(exchange.ToString());
            var parameter = manager.GetExchangeParameter();
            if (parameter != null)
            {
                SetProperty(ref charmCutoff, DateTime.ParseExact(parameter.CharmStartTime, "HH:mm:ss", CultureInfo.InvariantCulture));
                var holidays = new List<HolidayItem>();
                foreach (var holiday in parameter.Holidays)
                {
                    holidays.Add(new HolidayItem(this.SetModified, DateTime.ParseExact(holiday.Date, "yyyyMMdd", CultureInfo.InvariantCulture), holiday.Weight));
                }
                holidays.Sort((x, y) => x.Holiday.CompareTo(y.Holiday));
                Holidays = new ObservableCollection<HolidayItem>(holidays);
                SelectedHoliday = Holidays.Count - 1;

                var tradingSessions = new ObservableCollection<TradingSession>();
                foreach (var session in parameter.Sessions)
                {
                    tradingSessions.Add(new TradingSession(this.SetModified,
                        DateTime.ParseExact(session.Begin, "HH:mm:ss", CultureInfo.InvariantCulture),
                        DateTime.ParseExact(session.End, "HH:mm:ss", CultureInfo.InvariantCulture),
                        DateTime.ParseExact(session.Stop, "HH:mm:ss", CultureInfo.InvariantCulture)));
                }
                TradingSessions = tradingSessions;

                var maturityTradingSessions = new ObservableCollection<TradingSession>();
                foreach (var session in parameter.MaturitySessions)
                {
                    maturityTradingSessions.Add(new TradingSession(this.SetModified,
                        DateTime.ParseExact(session.Begin, "HH:mm:ss", CultureInfo.InvariantCulture),
                        DateTime.ParseExact(session.End, "HH:mm:ss", CultureInfo.InvariantCulture),
                        DateTime.ParseExact(session.Stop, "HH:mm:ss", CultureInfo.InvariantCulture)));
                }
                MaturityTradingSessions = maturityTradingSessions;

                UnsetModified();
            }
        }

        private void ModifyExecute()
        {
            Proto.ExchangeParameterReq req = new Proto.ExchangeParameterReq();
            req.Type = Proto.RequestType.Set;
            req.Exchange = this.exchange;

            Proto.ExchangeParameter p = new Proto.ExchangeParameter();
            p.Exchange = this.Exchange;
            p.CharmStartTime = this.CharmCutoff.ToString("HH:mm:ss");

            foreach (TradingSession session in TradingSessions)
            {
                Proto.TradingSession s = new Proto.TradingSession();
                s.Begin = session.Begin.ToString("HH:mm:ss");
                s.End = session.End.ToString("HH:mm:ss");
                s.Stop = session.Stop.ToString("HH:mm:ss");
                p.Sessions.Add(s);
            }

            foreach (TradingSession session in MaturityTradingSessions)
            {
                Proto.TradingSession s = new Proto.TradingSession();
                s.Begin = session.Begin.ToString("HH:mm:ss");
                s.End = session.End.ToString("HH:mm:ss");
                s.Stop = session.Stop.ToString("HH:mm:ss");
                p.MaturitySessions.Add(s);
            }

            foreach (HolidayItem holiday in Holidays)
            {
                Proto.Holiday h = new Proto.Holiday();
                h.Date = holiday.Holiday.ToString("yyyyMMdd");
                h.Weight = holiday.Weight;
                p.Holidays.Add(h);
            }

            req.Parameters.Add(p);

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

        private void AddHolidayExecute()
        {
            Holidays.Add(new HolidayItem(this.SetModified, DateTime.Now, 0));
            SelectedHoliday = Holidays.Count - 1;
            SetModified();
        }

        private void DeleteHolidayExecute()
        {
            int count = Holidays.Count;
            if (SelectedHoliday < count)
            {
                Holidays.RemoveAt(SelectedHoliday);
                if (SelectedHoliday == count - 1)
                {
                    --SelectedHoliday;
                }
            }
            SetModified();
        }

        private void AddTradingSessionExecute()
        {
            TradingSessions.Add(new TradingSession(this.SetModified, DateTime.Now, DateTime.Now, DateTime.Now));
            SelectedTradingSession = TradingSessions.Count - 1;
            SetModified();
        }

        private void DeleteTradingSessionExecute()
        {
            int count = TradingSessions.Count;
            if (SelectedTradingSession < count)
            {
                TradingSessions.RemoveAt(SelectedTradingSession);
                if (SelectedTradingSession == count - 1)
                {
                    --SelectedTradingSession;
                }
            }
            SetModified();
        }

        private void AddMaturityTradingSessionExecute()
        {
            MaturityTradingSessions.Add(new TradingSession(this.SetModified, DateTime.Now, DateTime.Now, DateTime.Now));
            SelectedMaturityTradingSession = MaturityTradingSessions.Count - 1;
            SetModified();
        }

        private void DeleteMaturityTradingSessionExecute()
        {
            int count = MaturityTradingSessions.Count;
            if (SelectedMaturityTradingSession < count)
            {
                MaturityTradingSessions.RemoveAt(SelectedMaturityTradingSession);
                if (SelectedMaturityTradingSession == count - 1)
                {
                    --SelectedMaturityTradingSession;
                }
            }
            SetModified();
        }
    }

    class HolidayItem : BindableBase
    {
        private DateTime holiday;
        public DateTime Holiday
        {
            get { return holiday; }
            set
            {
                if (SetProperty(ref holiday, value))
                {
                    this.action();
                }
            }
        }

        private double weight;
        public double Weight
        {
            get { return weight; }
            set
            {
                if (SetProperty(ref weight, value))
                {
                    this.action();
                }
            }
        }  
      
        public HolidayItem(Action action, DateTime holiday, double weight)
        {
            this.holiday = holiday;
            this.weight = weight;
            this.action = action;
        }

        private Action action;
    }

    class TradingSession : BindableBase
    {
        private DateTime begin;
        public DateTime Begin
        {
            get { return begin; }
            set
            {
                if (SetProperty(ref begin, value))
                {
                    this.action();
                }
            }
        }

        private DateTime end;
        public DateTime End
        {
            get { return end; }
            set
            {
                if (SetProperty(ref end, value))
                {
                    this.action();
                }
            }
        }

        private DateTime stop;
        public DateTime Stop
        {
            get { return stop; }
            set
            {
                if (SetProperty(ref stop, value))
                {
                    this.action();
                }
            }
        }

        public TradingSession(Action action, DateTime begin, DateTime end, DateTime stop)
        {
            this.begin = begin;
            this.end = end;
            this.stop = stop;

            this.action = action;
        }

        private Action action;
    }
}
