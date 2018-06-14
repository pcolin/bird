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
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;
using System.Xml;

namespace client.Views
{
    /// <summary>
    /// Interaction logic for StrategyWindow.xaml
    /// </summary>
    public partial class StrategyWindow : Window
    {
        public StrategyWindow(IUnityContainer container)
        {
            InitializeComponent();

            this.container = container;
            //this.DataContext = new StrategyWindowViewModel(container);
            container.Resolve<EventAggregator>().GetEvent<PubSubEvent<List<Proto.Exchange>>>().Subscribe(this.StartWindow, ThreadOption.PublisherThread);
        }

        public void SaveAndClose(XmlWriter writer)
        {
            MainWindow.WriteWindowPlacement(writer, this, "StrategyWindow");

            this.close = true;
            this.Close();
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

        private void StartWindow(List<Proto.Exchange> exchanges)
        {
            //var vm = this.DataContext as StrategyWindowViewModel;
            foreach (var exch in exchanges)
            {
                //var vm = new StrategyUserControlViewModel(this.container, this.Dispatcher, exch);
                var control = new StrategyUserControl(exch, this.container);
                TabItem item = new TabItem() { Header = exch, Content = control };
                this.ExchangeStrategyTabControl.Items.Add(item);
            }
            if (exchanges.Count > 0)
            {
                this.ExchangeStrategyTabControl.SelectedIndex = 0;
            }
        }

        private bool close = false;
        private IUnityContainer container;
    }
}
