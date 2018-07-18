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
    /// Interaction logic for QuoterUserControl.xaml
    /// </summary>
    public partial class QuoterUserControl : UserControl
    {
        public QuoterUserControl(Proto.Exchange exchange, IUnityContainer container)
        {
            InitializeComponent();

            var vm = new QuoterUserControlViewModel(exchange, container, this.Dispatcher);
            this.DataContext = vm;

            this.PricerColumn.ItemsSource = vm.GetPricers();
        }

        private void AddMenuItem_Click(object sender, RoutedEventArgs e)
        {
            var vm = this.DataContext as QuoterUserControlViewModel;
            QuoterSettingWindow w = new QuoterSettingWindow(vm);
            w.Owner = Window.GetWindow(this);
            w.ShowInTaskbar = false;
            w.ShowDialog();
            this.PricerColumn.ItemsSource = vm.GetPricers();
        }
    }
}
