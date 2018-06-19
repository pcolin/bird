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
    /// Interaction logic for PricerUserControl.xaml
    /// </summary>
    public partial class PricerUserControl : UserControl
    {
        public PricerUserControl(Proto.Exchange exchange, IUnityContainer container)
        {
            InitializeComponent();

            var vm = new PricerUserControlViewModel(exchange, container, this.Dispatcher);
            this.DataContext = vm;

            this.UnderlyingColumn.ItemsSource = vm.GetHedgeUnderlyings();
        }

        //private void EditMenuItem_Click(object sender, RoutedEventArgs e)
        //{
        //    //UnsetReadOnly();
        //    this.OptionDataGrid.IsReadOnly = false;
        //}

        //private void RefreshMenuItem_Click(object sender, RoutedEventArgs e)
        //{
        //    //SetReadOnly();
        //    this.OptionDataGrid.IsReadOnly = true;
        //}

        private void AddMenuItem_Click(object sender, RoutedEventArgs e)
        {
            PricerSettingWindow w = new PricerSettingWindow(this.DataContext as PricerUserControlViewModel);
            w.Owner = Window.GetWindow(this);
            w.ShowInTaskbar = false;
            w.ShowDialog();
        }

        //private void SetReadOnly()
        //{
        //    this.PricerDataGrid.IsReadOnly = true;
        //    this.OptionDataGrid.IsReadOnly = true;
        //    this.AddMenuItem.IsEnabled = false;
        //}

        //private void UnsetReadOnly()
        //{
        //    this.PricerDataGrid.IsReadOnly = false;
        //    this.OptionDataGrid.IsReadOnly = false;
        //}
    }

    public class OppositeBoolConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            bool v = (bool)value;
            return !v;
        }

        public object ConvertBack(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
}
