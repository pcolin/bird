using client.ViewModels;
using Microsoft.Practices.Unity;
using Prism.Events;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Forms;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;
using System.Xml;

namespace client.Views
{
    /// <summary>
    /// Interaction logic for TradingWindow.xaml
    /// </summary>
    public partial class TradingWindow : Window
    {
        public TradingWindow(Proto.Exchange exchange, IUnityContainer container)
        {
            InitializeComponent();
            this.exchange = exchange;
            this.container = container;

            container.Resolve<EventAggregator>().GetEvent<PubSubEvent<List<Proto.Exchange>>>().Subscribe(this.StartWindow, ThreadOption.PublisherThread);
        }

        public void SaveAndClose(XmlWriter writer)
        {
            this.Dispatcher.Invoke(() =>
            {
                MainWindow.WriteWindowPlacement(writer, this, "TradingWindow_" + this.exchange);

                this.close = true;
                this.Close();
            });
        }

        private void StartWindow(List<Proto.Exchange> exchanges)
        {
            this.Dispatcher.BeginInvoke((MethodInvoker)delegate
            {
                if (exchanges.Contains(this.exchange))
                {
                    this.DataContext = new TradingWindowViewModel(exchange, container, this.Dispatcher);
                }
                else
                {
                    this.Close();
                }
            });
        }

        private void Window_Closing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            if (close == false)
            {
                this.Hide();
                e.Cancel = true;
                return;
            }
        }

        private bool close = false;
        private Proto.Exchange exchange;
        private IUnityContainer container;

        private void RadioButton1_Checked(object sender, RoutedEventArgs e)
        {
            this.VolumeIntegerUpDown.Value = 1;
        }

        private void RadioButton2_Checked(object sender, RoutedEventArgs e)
        {
            this.VolumeIntegerUpDown.Value = 2;
        }

        private void RadioButton5_Checked(object sender, RoutedEventArgs e)
        {
            this.VolumeIntegerUpDown.Value = 5;
        }

        private void RadioButton10_Checked(object sender, RoutedEventArgs e)
        {
            this.VolumeIntegerUpDown.Value = 10;
        }

        private void RadioButton20_Checked(object sender, RoutedEventArgs e)
        {
            this.VolumeIntegerUpDown.Value = 20;
        }

        private void RadioButton50_Checked(object sender, RoutedEventArgs e)
        {
            this.VolumeIntegerUpDown.Value = 50;
        }
    }
    
    public class VolumeConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            int v = (int)value;
            if (v > 0)
            {
                return v;
            }
            else
            {
                return null;
            }
        }

        public object ConvertBack(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class VolumeToColorConverter : IMultiValueConverter
    {
        public object Convert(object[] values, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            int bidVolume = (int)values[0];
            int askVolume = (int)values[1];
            if (bidVolume > 0)
            {
                return Brushes.Red;
            }
            else if (askVolume > 0)
            {
                return Brushes.Green;
            }
            else
            {
                bidVolume = (int)values[2];
                if (bidVolume > 0)
                {
                    return Brushes.Red;
                }
                else
                {
                    return Brushes.Green;
                }
            }
        }

        public object[] ConvertBack(object value, Type[] targetTypes, object parameter, System.Globalization.CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }

    public class NetToStringMultiConverter : IMultiValueConverter
    {
        public object Convert(object[] values, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            if (values[0] is double && values[1] is double)
            {
                double value1 = (double)values[0];
                double value2 = (double)values[1];
                if (double.IsNaN(value1) == false && double.IsNaN(value2) == false)
                {
                    return (value1 - value2).ToString("F2");
                }
            }
            return null;
        }

        public object[] ConvertBack(object value, Type[] targetTypes, object parameter, System.Globalization.CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }


    public class NetPercentToStringMultiConverter : IMultiValueConverter
    {
        public object Convert(object[] values, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            if (values[0] is double && values[1] is double)
            {
                double value1 = (double)values[0];
                double value2 = (double)values[1];
                if (double.IsNaN(value1) == false && double.IsNaN(value2) == false)
                {
                    double value = value1 / value2 - 1;
                    return (value * 100).ToString("F2") + "%";
                }
            }
            return null;
        }

        public object[] ConvertBack(object value, Type[] targetTypes, object parameter, System.Globalization.CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
}
