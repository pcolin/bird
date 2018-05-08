using Prism.Mvvm;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace client.ViewModels
{
    class ExchangeUserControlViewModel : BindableBase
    {
        private Proto.Exchange exchange;
        public Proto.Exchange Exchange
        {
            get { return exchange; }
            set { SetProperty(ref exchange, value); }
        }

        private string charmCutOff;
        public string CharmCutOff
        {
            get { return charmCutOff; }
            set { SetProperty(ref charmCutOff, value); }
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

        private ObservableCollection<HolidayItem> holidays;
        public ObservableCollection<HolidayItem> Holidays
        {
            get { return holidays; }
            set { SetProperty(ref holidays, value); }
        }        
    }

    class HolidayItem : BindableBase
    {
        private DateTime holiday;
        public DateTime Holiday
        {
            get { return holiday; }
            set { SetProperty(ref holiday, value); }
        }

        private double weight;
        public double Weight
        {
            get { return weight; }
            set { SetProperty(ref weight, value); }
        }        
    }
}
