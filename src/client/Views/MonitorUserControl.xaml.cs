using client.ViewModels;
using Microsoft.Practices.Unity;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace client.Views
{
    /// <summary>
    /// Interaction logic for MonitorUserControl.xaml
    /// </summary>
    public partial class MonitorUserControl : UserControl
    {
        public MonitorUserControl(MonitorUserControlViewModel viewModel)
        {
            InitializeComponent();
            this.DataContext = viewModel;
            this.ListBoxUnderlyings.ItemsSource = viewModel.UnderlyingsView;
            this.ListBoxTypes.ItemsSource = viewModel.TypesView;
        }
    }

    public class StatusToColorConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            Proto.StrategyStatus status = (Proto.StrategyStatus)value;
            return status == Proto.StrategyStatus.Running ? Brushes.Green : Brushes.Red;
        }

        public object ConvertBack(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
}
